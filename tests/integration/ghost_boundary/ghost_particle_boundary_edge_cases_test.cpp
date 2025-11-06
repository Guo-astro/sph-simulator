// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file ghost_particle_boundary_edge_cases_test.cpp
 * @brief BDD-style tests for ghost particle boundary edge cases
 * 
 * These tests were written to prevent regression of critical bugs:
 * Bug 1: Density under/over-estimation at boundaries due to incorrect ghost positioning
 * Bug 2: Ghost particles having opposite velocity (running away) due to wrong reflection
 */

#include <gtest/gtest.h>
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "defines.hpp"

using namespace sph;

/**
 * BDD Test Suite: Ghost Particle Boundary Edge Cases
 * 
 * Feature: Accurate ghost particle generation for boundary conditions
 * As a: SPH simulation user
 * I want: Ghost particles to correctly support density calculations at boundaries
 * So that: Density remains accurate across the entire domain
 */
class GhostParticleBoundaryEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }
    
    /**
     * Helper: Count ghost particles in a specific range
     */
    int countGhostsInRange(const std::vector<SPHParticle<1>>& ghosts, 
                          real min_x, real max_x) {
        int count = 0;
        for (const auto& ghost : ghosts) {
            if (ghost.pos[0] >= min_x && ghost.pos[0] <= max_x) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * Helper: Find ghost particle matching position within tolerance
     */
    bool hasGhostNear(const std::vector<SPHParticle<1>>& ghosts,
                     real target_x, real tolerance = 1e-6) {
        for (const auto& ghost : ghosts) {
            if (std::abs(ghost.pos[0] - target_x) < tolerance) {
                return true;
            }
        }
        return false;
    }
};

/**
 * Scenario: Particle exactly at lower boundary should create ghost at upper side
 * 
 * Given: A 1D periodic domain from -0.5 to 1.5 (range = 2.0)
 * And: A particle positioned exactly at x = -0.5 (lower boundary)
 * And: Kernel support radius of 0.2
 * When: Ghost particles are generated
 * Then: A ghost particle should exist at x = 1.5 (upper side)
 * And: The ghost should have the same velocity as the real particle
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, ParticleAtLowerBoundary_CreatesGhostAtUpperSide) {
    // Given: 1D periodic domain [-0.5, 1.5]
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Particle exactly at lower boundary with positive velocity
    std::vector<SPHParticle<1>> real_particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{-0.5};  // Exactly at lower boundary
    p.vel = Vector<1>{1.0};   // Moving right
    p.dens = 1.0;
    p.pres = 0.1;
    p.mass = 1.0;
    p.sml = 0.1;
    p.type = static_cast<int>(ParticleType::REAL);
    real_particles.push_back(p);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: Ghost exists at upper side (x = 1.5)
    EXPECT_GT(ghosts.size(), 0) << "Should create at least one ghost particle";
    EXPECT_TRUE(hasGhostNear(ghosts, 1.5)) 
        << "Ghost should exist at x=1.5 (upper boundary) for particle at x=-0.5";
    
    // And: Ghost has same velocity (periodic, not reflected)
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[0] - 1.5) < 1e-6) {
            EXPECT_NEAR(ghost.vel[0], 1.0, 1e-6) 
                << "Periodic ghost should have SAME velocity as real particle";
            EXPECT_GT(ghost.vel[0], 0.0) 
                << "Velocity should be positive (moving right), not negative";
        }
    }
}

/**
 * Scenario: Particle exactly at upper boundary should create ghost at lower side
 * 
 * Given: A 1D periodic domain from -0.5 to 1.5
 * And: A particle positioned exactly at x = 1.5 (upper boundary)
 * When: Ghost particles are generated
 * Then: A ghost particle should exist at x = -0.5 (lower side)
 * And: The ghost should have the same velocity as the real particle
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, ParticleAtUpperBoundary_CreatesGhostAtLowerSide) {
    // Given: 1D periodic domain
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Particle exactly at upper boundary
    std::vector<SPHParticle<1>> real_particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{1.5};   // Exactly at upper boundary
    p.vel = Vector<1>{-1.0};  // Moving left
    p.dens = 1.0;
    p.pres = 0.1;
    p.mass = 1.0;
    p.sml = 0.1;
    p.type = static_cast<int>(ParticleType::REAL);
    real_particles.push_back(p);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: Ghost exists at lower side
    EXPECT_GT(ghosts.size(), 0) << "Should create at least one ghost particle";
    EXPECT_TRUE(hasGhostNear(ghosts, -0.5)) 
        << "Ghost should exist at x=-0.5 (lower boundary) for particle at x=1.5";
    
    // And: Ghost has same velocity
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[0] - (-0.5)) < 1e-6) {
            EXPECT_NEAR(ghost.vel[0], -1.0, 1e-6) 
                << "Periodic ghost should have SAME velocity as real particle";
            EXPECT_LT(ghost.vel[0], 0.0) 
                << "Velocity should be negative (moving left), not positive";
        }
    }
}

/**
 * Scenario: Particle very close to boundary (within kernel) should create ghost
 * 
 * Given: A 1D periodic domain from -0.5 to 1.5
 * And: Kernel support radius of 0.2
 * And: A particle at x = -0.48 (0.02 from lower boundary, within kernel support)
 * When: Ghost particles are generated
 * Then: A ghost particle should exist near x = 1.52 (wrapped to upper side)
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, ParticleNearBoundary_CreatesGhost) {
    // Given: Domain and kernel support
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Particle close to lower boundary (within kernel support)
    std::vector<SPHParticle<1>> real_particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{-0.48};  // 0.02 from boundary, within 0.2 kernel support
    p.vel = Vector<1>{1.0};
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    real_particles.push_back(p);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: Ghost exists at upper side
    EXPECT_GT(ghosts.size(), 0) << "Particle within kernel support should generate ghost";
    
    // Ghost should be at: -0.48 + 2.0 = 1.52
    EXPECT_TRUE(hasGhostNear(ghosts, 1.52, 0.01)) 
        << "Ghost should exist at x=1.52 for particle at x=-0.48";
}

/**
 * Scenario: Particle far from boundary should NOT create ghost
 * 
 * Given: Kernel support radius of 0.2
 * And: A particle at x = 0.0 (0.5 from lower boundary, 1.5 from upper)
 * When: Ghost particles are generated
 * Then: No ghost particles should be created for this particle
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, ParticleFarFromBoundary_NoGhost) {
    // Given: Domain and kernel support
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Particle far from both boundaries
    std::vector<SPHParticle<1>> real_particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{0.0};  // Center of domain
    p.vel = Vector<1>{0.0};
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    real_particles.push_back(p);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: No ghosts created
    EXPECT_EQ(ghosts.size(), 0) 
        << "Particle far from boundaries should not generate ghosts";
}

/**
 * Scenario: Mirror boundary with NO_SLIP should reflect ALL velocity components
 * 
 * Given: A 1D mirror boundary at x = 0.0 with NO_SLIP condition
 * And: A particle at x = 0.05 moving toward the wall with velocity v = -1.0
 * When: Ghost particles are generated
 * Then: Ghost particle should exist at x = -0.05 (mirrored position)
 * And: Ghost velocity should be +1.0 (fully reflected for no-slip)
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, MirrorBoundary_NoSlip_ReflectsVelocity) {
    // Given: Mirror boundary with no-slip
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;
    config.mirror_types[0] = MirrorType::NO_SLIP;
    config.range_min = Vector<1>{0.0};
    config.range_max = Vector<1>{1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Particle near wall moving toward it
    std::vector<SPHParticle<1>> real_particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{0.05};   // Near lower boundary
    p.vel = Vector<1>{-1.0};   // Moving toward wall
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    real_particles.push_back(p);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: Ghost at mirrored position
    EXPECT_GT(ghosts.size(), 0) << "Should create ghost for mirror boundary";
    EXPECT_TRUE(hasGhostNear(ghosts, -0.05)) 
        << "Ghost should be at mirrored position x=-0.05";
    
    // And: Velocity fully reflected (no-slip)
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[0] - (-0.05)) < 1e-6) {
            EXPECT_NEAR(ghost.vel[0], 1.0, 1e-6) 
                << "No-slip mirror should fully reflect velocity: v_ghost = -v_real";
        }
    }
}

/**
 * Scenario: Periodic boundary should NEVER reflect velocity
 * 
 * Given: A periodic domain
 * And: Multiple particles near boundaries with various velocities
 * When: Ghost particles are generated
 * Then: All ghosts should have SAME velocity as their source particles
 * And: No velocity component should be negated
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, PeriodicBoundary_NeverReflectsVelocity) {
    // Given: Periodic domain
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.2);
    
    // And: Multiple particles with different velocities
    std::vector<SPHParticle<1>> real_particles;
    
    SPHParticle<1> p1;
    p1.pos = Vector<1>{-0.49}; p1.vel = Vector<1>{2.5}; p1.dens = 1.0; p1.mass = 1.0;
    p1.type = static_cast<int>(ParticleType::REAL);
    
    SPHParticle<1> p2;
    p2.pos = Vector<1>{1.49}; p2.vel = Vector<1>{-3.7}; p2.dens = 1.0; p2.mass = 1.0;
    p2.type = static_cast<int>(ParticleType::REAL);
    
    SPHParticle<1> p3;
    p3.pos = Vector<1>{-0.45}; p3.vel = Vector<1>{0.0}; p3.dens = 1.0; p3.mass = 1.0;
    p3.type = static_cast<int>(ParticleType::REAL);
    
    real_particles.push_back(p1);
    real_particles.push_back(p2);
    real_particles.push_back(p3);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: All ghosts have same velocity as source
    EXPECT_GT(ghosts.size(), 0) << "Should create ghosts for boundary particles";
    
    for (const auto& ghost : ghosts) {
        // Find the source particle
        for (const auto& real_p : real_particles) {
            // Check if this ghost likely came from this real particle
            real expected_ghost_pos = real_p.pos[0];
            if (real_p.pos[0] < config.range_min[0] + 0.2) {
                expected_ghost_pos += (config.range_max[0] - config.range_min[0]);
            } else if (real_p.pos[0] > config.range_max[0] - 0.2) {
                expected_ghost_pos -= (config.range_max[0] - config.range_min[0]);
            } else {
                continue;
            }
            
            if (std::abs(ghost.pos[0] - expected_ghost_pos) < 1e-3) {
                EXPECT_NEAR(ghost.vel[0], real_p.vel[0], 1e-6)
                    << "Periodic ghost velocity should match real particle velocity exactly";
                    
                // Check sign preservation
                if (real_p.vel[0] > 0) {
                    EXPECT_GT(ghost.vel[0], 0.0) 
                        << "Positive velocity should remain positive in periodic ghost";
                } else if (real_p.vel[0] < 0) {
                    EXPECT_LT(ghost.vel[0], 0.0) 
                        << "Negative velocity should remain negative in periodic ghost";
                }
            }
        }
    }
}

/**
 * Scenario: Shock tube boundary particles should have proper ghost support
 * 
 * Given: Shock tube domain [-0.5, 1.5] with periodic boundaries
 * And: Particles exactly at x = -0.5 and x = 1.5
 * And: Kernel support radius typical for SPH (2h where h ≈ 0.02)
 * When: Ghost particles are generated
 * Then: Both boundary particles should have ghosts on opposite sides
 * And: Density calculation should include contributions from ghosts
 */
TEST_F(GhostParticleBoundaryEdgeCasesTest, ShockTubeBoundaries_ProperGhostSupport) {
    // Given: Shock tube configuration
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    // Typical kernel support for 100 particles in length 2.0
    // h ≈ 2.0/100 = 0.02, kernel support ≈ 2h = 0.04
    manager.set_kernel_support_radius(0.04);
    
    // And: Particles at both boundaries
    std::vector<SPHParticle<1>> real_particles;
    
    SPHParticle<1> p_left;
    p_left.pos = Vector<1>{-0.5};
    p_left.vel = Vector<1>{1.0};
    p_left.dens = 1.0;
    p_left.mass = 1.0;
    p_left.sml = 0.02;
    p_left.type = static_cast<int>(ParticleType::REAL);
    
    SPHParticle<1> p_right;
    p_right.pos = Vector<1>{1.5};
    p_right.vel = Vector<1>{-1.0};
    p_right.dens = 1.0;
    p_right.mass = 1.0;
    p_right.sml = 0.02;
    p_right.type = static_cast<int>(ParticleType::REAL);
    
    real_particles.push_back(p_left);
    real_particles.push_back(p_right);
    
    // When: Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    // Then: Ghosts exist for both boundaries
    EXPECT_GE(ghosts.size(), 2) 
        << "Both boundary particles should generate ghosts";
    
    // Left particle ghost should be at right side
    bool has_left_ghost = hasGhostNear(ghosts, 1.5, 0.01);
    EXPECT_TRUE(has_left_ghost) 
        << "Left boundary particle (-0.5) should have ghost at right (1.5)";
    
    // Right particle ghost should be at left side  
    bool has_right_ghost = hasGhostNear(ghosts, -0.5, 0.01);
    EXPECT_TRUE(has_right_ghost)
        << "Right boundary particle (1.5) should have ghost at left (-0.5)";
}

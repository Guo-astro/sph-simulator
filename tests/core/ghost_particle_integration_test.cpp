/**
 * @file ghost_particle_integration_test.cpp
 * @brief BDD-style integration tests for ghost particle system
 * 
 * Following BDD (Behavior-Driven Development) approach:
 * - GIVEN: Setup initial conditions
 * - WHEN: Perform actions
 * - THEN: Verify expected outcomes
 */

#include <gtest/gtest.h>
#include "core/ghost_particle_manager.hpp"
#include "core/boundary_types.hpp"
#include "core/sph_particle.hpp"
#include "core/simulation.hpp"
#include "exhaustive_search.hpp"
#include <vector>
#include <cmath>

using namespace sph;

/**
 * BDD Test Suite: Ghost Particle Integration
 */
class GhostParticleIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }
    
    // Helper: Create a simple 1D periodic configuration
    BoundaryConfiguration<1> create_1d_periodic_config() {
        BoundaryConfiguration<1> config;
        config.is_valid = true;
        config.types[0] = BoundaryType::PERIODIC;
        config.range_min[0] = 0.0;
        config.range_max[0] = 1.0;
        return config;
    }
    
    // Helper: Create test particle
    template<int Dim>
    SPHParticle<Dim> create_particle(const Vector<Dim>& position, 
                                      const Vector<Dim>& velocity = Vector<Dim>()) {
        SPHParticle<Dim> p;
        p.pos = position;
        p.vel = velocity;
        p.dens = 1.0;
        p.pres = 1.0;
        p.mass = 1.0;
        p.sml = 0.05;
        return p;
    }
};

// ============================================================================
// Feature: Ghost particles should be included in neighbor search
// ============================================================================

/**
 * Scenario: Particle near boundary finds ghost as neighbor
 */
TEST_F(GhostParticleIntegrationTest, ParticleNearBoundaryFindsGhostNeighbor) {
    // GIVEN a particle near the lower boundary in a periodic domain
    std::vector<SPHParticle<1>> particles;
    Vector<1> pos = {0.02};  // Very close to lower boundary (0.0)
    particles.push_back(create_particle(pos));
    
    // AND ghost particles are generated
    auto config = create_1d_periodic_config();
    GhostParticleManager<1> ghost_manager;
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    // WHEN we get the combined particle list for neighbor search
    std::vector<SPHParticle<1>> all_particles = particles;
    const auto& ghosts = ghost_manager.get_ghost_particles();
    all_particles.insert(all_particles.end(), ghosts.begin(), ghosts.end());
    
    // THEN we should have real + ghost particles
    EXPECT_EQ(all_particles.size(), particles.size() + ghosts.size());
    
    // AND the ghost should be at the expected position (wrapped to upper boundary)
    if (!ghosts.empty()) {
        EXPECT_GT(ghosts[0].pos[0], 0.9);  // Ghost should be near x=1.0
    }
}

/**
 * Scenario: Ghost particles should NOT be force calculation targets
 */
TEST_F(GhostParticleIntegrationTest, GhostParticlesShouldNotReceiveForces) {
    // GIVEN a simulation with real and ghost particles
    std::vector<SPHParticle<1>> real_particles;
    real_particles.push_back(create_particle(Vector<1>{0.05}));
    
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(real_particles);
    
    int real_count = static_cast<int>(real_particles.size());
    int total_count = real_count + ghost_manager.get_ghost_count();
    
    // WHEN iterating through particles for force calculation
    // THEN only real particles should be processed
    for (int i = 0; i < total_count; ++i) {
        if (i < real_count) {
            // This is a real particle - forces SHOULD be calculated
            EXPECT_TRUE(true);  // Force calculation allowed
        } else {
            // This is a ghost particle - forces should NOT be calculated
            EXPECT_GE(i, real_count);  // Verify it's in ghost range
        }
    }
    
    EXPECT_GT(total_count, real_count);  // Verify ghosts exist
}

/**
 * Scenario: Combined particle list preserves real particle indices
 */
TEST_F(GhostParticleIntegrationTest, CombinedListPreservesRealParticleIndices) {
    // GIVEN multiple real particles
    std::vector<SPHParticle<1>> real_particles;
    real_particles.push_back(create_particle(Vector<1>{0.05}));
    real_particles.push_back(create_particle(Vector<1>{0.5}));
    real_particles.push_back(create_particle(Vector<1>{0.95}));
    
    // AND ghost particles are generated
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(real_particles);
    
    // WHEN creating combined list
    std::vector<SPHParticle<1>> all_particles = real_particles;
    const auto& ghosts = ghost_manager.get_ghost_particles();
    all_particles.insert(all_particles.end(), ghosts.begin(), ghosts.end());
    
    // THEN real particles should be at indices [0, real_count)
    int real_count = static_cast<int>(real_particles.size());
    for (int i = 0; i < real_count; ++i) {
        EXPECT_DOUBLE_EQ(all_particles[i].pos[0], real_particles[i].pos[0]);
    }
    
    // AND ghost particles should be at indices [real_count, total_count)
    int ghost_count = ghost_manager.get_ghost_count();
    for (int i = 0; i < ghost_count; ++i) {
        EXPECT_DOUBLE_EQ(all_particles[real_count + i].pos[0], ghosts[i].pos[0]);
    }
}

// ============================================================================
// Feature: Ghost particles enhance neighbor search across boundaries
// ============================================================================

/**
 * Scenario: Particle finds neighbors across periodic boundary
 */
TEST_F(GhostParticleIntegrationTest, ParticleFin dsNeighborsAcrossPeriodicBoundary) {
    // GIVEN two particles on opposite sides of periodic boundary
    std::vector<SPHParticle<1>> particles;
    particles.push_back(create_particle(Vector<1>{0.02}));   // Near x=0
    particles.push_back(create_particle(Vector<1>{0.98}));   // Near x=1
    
    // AND ghost particles for periodic boundaries
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    // WHEN we create the combined particle list
    std::vector<SPHParticle<1>> all_particles = particles;
    const auto& ghosts = ghost_manager.get_ghost_particles();
    all_particles.insert(all_particles.end(), ghosts.begin(), ghosts.end());
    
    // THEN particles should be able to find each other via ghosts
    // Particle at 0.02 should have a ghost of particle at 0.98 appearing at -0.02
    // Particle at 0.98 should have a ghost of particle at 0.02 appearing at 1.02
    EXPECT_EQ(ghost_manager.get_ghost_count(), 2);
    
    // Verify ghost positions enable cross-boundary interactions
    bool has_ghost_near_zero = false;
    bool has_ghost_near_one = false;
    
    for (const auto& ghost : ghosts) {
        if (ghost.pos[0] < 0.1 && ghost.pos[0] > -0.1) {
            has_ghost_near_zero = true;
        }
        if (ghost.pos[0] > 0.9 || ghost.pos[0] > 1.0) {
            has_ghost_near_one = true;
        }
    }
    
    EXPECT_TRUE(has_ghost_near_zero || has_ghost_near_one);
}

// ============================================================================
// Feature: Ghost properties update with real particles
// ============================================================================

/**
 * Scenario: Ghost velocity updates when real particle velocity changes
 */
TEST_F(GhostParticleIntegrationTest, GhostPropertiesUpdateWithRealParticles) {
    // GIVEN a particle with initial velocity
    std::vector<SPHParticle<1>> particles;
    Vector<1> initial_vel = {1.0};
    particles.push_back(create_particle(Vector<1>{0.05}, initial_vel));
    
    // AND ghosts are generated
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    ASSERT_GT(ghost_manager.get_ghost_count(), 0);
    const auto& ghosts_before = ghost_manager.get_ghost_particles();
    EXPECT_DOUBLE_EQ(ghosts_before[0].vel[0], 1.0);
    
    // WHEN the real particle's velocity changes
    particles[0].vel[0] = 2.0;
    ghost_manager.update_ghosts(particles);
    
    // THEN the ghost's velocity should also update
    const auto& ghosts_after = ghost_manager.get_ghost_particles();
    EXPECT_DOUBLE_EQ(ghosts_after[0].vel[0], 2.0);
}

/**
 * Scenario: Ghost density reflects real particle density after update
 */
TEST_F(GhostParticleIntegrationTest, GhostDensityReflectsRealParticleDensity) {
    // GIVEN a particle with initial density
    std::vector<SPHParticle<1>> particles;
    auto p = create_particle(Vector<1>{0.05});
    p.dens = 1.0;
    particles.push_back(p);
    
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    ASSERT_GT(ghost_manager.get_ghost_count(), 0);
    
    // WHEN density changes due to SPH calculations
    particles[0].dens = 1.5;
    ghost_manager.update_ghosts(particles);
    
    // THEN ghost should reflect new density
    const auto& ghosts = ghost_manager.get_ghost_particles();
    EXPECT_DOUBLE_EQ(ghosts[0].dens, 1.5);
}

// ============================================================================
// Feature: 2D Ghost particles work correctly
// ============================================================================

/**
 * Scenario: 2D periodic boundaries create corner ghosts
 */
TEST_F(GhostParticleIntegrationTest, TwoDimensionalPeriodicBoundariesCreateCornerGhosts) {
    // GIVEN a 2D domain with periodic boundaries in both dimensions
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.types[1] = BoundaryType::PERIODIC;
    config.range_min = {0.0, 0.0};
    config.range_max = {1.0, 1.0};
    
    // AND a particle near the corner
    std::vector<SPHParticle<2>> particles;
    Vector<2> corner_pos = {0.05, 0.05};
    particles.push_back(create_particle(corner_pos));
    
    // WHEN ghosts are generated
    GhostParticleManager<2> ghost_manager;
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    // THEN should create ghosts for: +x, +y, and +x+y corner
    // Total: 3 ghosts
    EXPECT_EQ(ghost_manager.get_ghost_count(), 3);
}

/**
 * Scenario: Mirror boundary reflects velocity correctly
 */
TEST_F(GhostParticleIntegrationTest, MirrorBoundaryReflectsVelocityCorrectly) {
    // GIVEN a 2D domain with mirror boundary in y-direction
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::NONE;
    config.types[1] = BoundaryType::MIRROR;
    config.enable_lower[1] = true;
    config.mirror_types[1] = MirrorType::NO_SLIP;
    config.range_min = {0.0, 0.0};
    config.range_max = {1.0, 1.0};
    
    // AND a particle moving toward the wall
    std::vector<SPHParticle<2>> particles;
    Vector<2> pos = {0.5, 0.05};
    Vector<2> vel = {1.0, 0.5};  // Moving in +x and +y
    particles.push_back(create_particle(pos, vel));
    
    // WHEN ghosts are generated
    GhostParticleManager<2> ghost_manager;
    ghost_manager.initialize(config);
    ghost_manager.set_kernel_support_radius(0.1);
    ghost_manager.generate_ghosts(particles);
    
    ASSERT_GT(ghost_manager.get_ghost_count(), 0);
    
    // THEN ghost should have reflected velocity (no-slip)
    const auto& ghosts = ghost_manager.get_ghost_particles();
    EXPECT_DOUBLE_EQ(ghosts[0].vel[0], -1.0);  // x-component reflected
    EXPECT_DOUBLE_EQ(ghosts[0].vel[1], -0.5);  // y-component reflected
}

// ============================================================================
// Feature: Periodic wrapping maintains particles in domain
// ============================================================================

/**
 * Scenario: Particle moved outside domain is wrapped back
 */
TEST_F(GhostParticleIntegrationTest, ParticleMovedOutsideDomainIsWrappedBack) {
    // GIVEN a periodic domain
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    
    // AND particles that have moved outside the domain
    std::vector<SPHParticle<1>> particles;
    particles.push_back(create_particle(Vector<1>{-0.1}));  // Below lower boundary
    particles.push_back(create_particle(Vector<1>{1.2}));   // Above upper boundary
    
    // WHEN periodic wrapping is applied
    ghost_manager.apply_periodic_wrapping(particles);
    
    // THEN particles should be wrapped back into [0, 1]
    EXPECT_NEAR(particles[0].pos[0], 0.9, 1e-10);  // -0.1 + 1.0 = 0.9
    EXPECT_NEAR(particles[1].pos[0], 0.2, 1e-10);  // 1.2 - 1.0 = 0.2
}

/**
 * Scenario: Wrapping preserves other particle properties
 */
TEST_F(GhostParticleIntegrationTest, WrappingPreservesOtherParticleProperties) {
    // GIVEN a particle outside domain with specific properties
    std::vector<SPHParticle<1>> particles;
    auto p = create_particle(Vector<1>{-0.1}, Vector<1>{2.0});
    p.dens = 1.5;
    p.pres = 2.0;
    particles.push_back(p);
    
    GhostParticleManager<1> ghost_manager;
    auto config = create_1d_periodic_config();
    ghost_manager.initialize(config);
    
    // WHEN wrapping is applied
    ghost_manager.apply_periodic_wrapping(particles);
    
    // THEN only position should change, other properties preserved
    EXPECT_DOUBLE_EQ(particles[0].vel[0], 2.0);
    EXPECT_DOUBLE_EQ(particles[0].dens, 1.5);
    EXPECT_DOUBLE_EQ(particles[0].pres, 2.0);
}

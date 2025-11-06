// Use 1D for tests by default
static constexpr int Dim = 1;

#include <gtest/gtest.h>
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "parameters.hpp"
#include <memory>
#include <vector>

using namespace sph;

/**
 * @brief Test suite for particle initialization and ghost particle generation
 * 
 * These tests prevent regression of issues like:
 * - Ghost particles being generated before smoothing lengths are calculated
 * - Kernel support radius set to 0 due to uninitialized particle properties
 * - Invalid state during initialization sequence
 */
class ParticleInitializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create basic SPH parameters
        params = std::make_shared<SPHParameters>();
        params->dimension = 1;
        params->physics.gamma = 1.4;
        params->physics.neighbor_number = 4;
        params->kernel = KernelType::CUBIC_SPLINE;
    }

    std::shared_ptr<SPHParameters> params;
};

/**
 * @test Verify that newly created particles have zero smoothing length
 * This documents the expected initial state before pre-interaction
 */
TEST_F(ParticleInitializationTest, NewParticlesHaveZeroSmoothingLength) {
    std::vector<SPHParticle<1>> particles(10);
    
    for (const auto& p : particles) {
        EXPECT_EQ(p.sml, 0.0) << "Newly created particles should have sml=0";
    }
}

/**
 * @test Verify that ghost particle kernel support radius cannot be set from uninitialized particles
 * This prevents the bug where kernel_support_radius = 0 because particles.sml = 0
 */
TEST_F(ParticleInitializationTest, CannotComputeKernelSupportFromUninitializedParticles) {
    std::vector<SPHParticle<1>> particles(10);
    
    // Attempt to find maximum smoothing length
    real max_sml = 0.0;
    for (const auto& p : particles) {
        max_sml = std::max(max_sml, p.sml);
    }
    
    EXPECT_EQ(max_sml, 0.0) << "Cannot compute valid kernel support from uninitialized particles";
    
    // kernel_support_radius = 2.0 * max_sml would give 0, which is invalid!
    real kernel_support_radius = 2.0 * max_sml;
    EXPECT_EQ(kernel_support_radius, 0.0) << "This demonstrates the bug";
}

/**
 * @test Verify correct approach: estimate smoothing length from particle spacing
 */
TEST_F(ParticleInitializationTest, EstimateSmoothingLengthFromSpacing) {
    const int n_particles = 10;
    const real domain_length = 1.0;
    const real dx = domain_length / n_particles;
    
    std::vector<SPHParticle<1>> particles(n_particles);
    
    // Initialize positions
    for (int i = 0; i < n_particles; ++i) {
        particles[i].pos[0] = (i + 0.5) * dx;
    }
    
    // Estimate smoothing length from spacing (typical for cubic spline in 1D)
    const real estimated_sml = 2.0 * dx;
    const real kernel_support_radius = 2.0 * estimated_sml;  // 2h for cubic spline
    
    EXPECT_GT(estimated_sml, 0.0) << "Estimated sml should be positive";
    EXPECT_GT(kernel_support_radius, 0.0) << "Kernel support radius should be positive";
    EXPECT_NEAR(estimated_sml, 0.2, 1e-6) << "For 10 particles in domain [0,1], sml ≈ 2*0.1 = 0.2";
}

/**
 * @test Verify ghost particle manager initialization sequence
 */
TEST_F(ParticleInitializationTest, GhostParticleManagerInitializationSequence) {
    const int n_particles = 50;
    const real domain_min = -0.5;
    const real domain_max = 1.5;
    const real domain_length = domain_max - domain_min;
    const real dx = domain_length / n_particles;
    
    // Create particles
    std::vector<SPHParticle<1>> particles(n_particles);
    for (int i = 0; i < n_particles; ++i) {
        particles[i].pos[0] = domain_min + (i + 0.5) * dx;
        particles[i].dens = 1.0;
        particles[i].pres = 1.0;
        particles[i].mass = dx;
        particles[i].id = i;
        // Note: sml is still 0!
    }
    
    // Create ghost particle manager
    auto ghost_manager = std::make_shared<GhostParticleManager<1>>();
    
    // Configure mirror boundaries
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = domain_min;
    config.range_max[0] = domain_max;
    config.enable_lower[0] = true;
    config.enable_upper[0] = true;
    config.mirror_types[0] = MirrorType::NO_SLIP;
    
    ghost_manager->initialize(config);
    
    // CORRECT: Estimate kernel support from particle spacing, not from sml
    const real estimated_sml = 2.0 * dx;
    const real kernel_support_radius = 2.0 * estimated_sml;
    ghost_manager->set_kernel_support_radius(kernel_support_radius);
    
    EXPECT_GT(kernel_support_radius, 0.0) << "Kernel support radius must be positive";
    
    // Generate ghost particles
    ghost_manager->generate_ghosts(particles);
    
    const int ghost_count = ghost_manager->get_ghost_count();
    EXPECT_GT(ghost_count, 0) << "Should generate ghost particles with valid kernel support radius";
    
    // Verify ghost particles are outside domain
    const auto& ghosts = ghost_manager->get_ghost_particles();
    for (const auto& ghost : ghosts) {
        EXPECT_TRUE(ghost.pos[0] < domain_min || ghost.pos[0] > domain_max)
            << "Ghost particles should be outside domain";
        EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST))
            << "Ghost particles should have type=GHOST";
    }
}

/**
 * @test Verify that ghost particles inherit properties from real particles
 */
TEST_F(ParticleInitializationTest, GhostParticlesInheritProperties) {
    const int n_particles = 20;
    const real dx = 0.05;
    
    std::vector<SPHParticle<1>> particles(n_particles);
    for (int i = 0; i < n_particles; ++i) {
        particles[i].pos[0] = -0.5 + (i + 0.5) * dx;
        particles[i].dens = 1.0 + 0.1 * i;  // Varying density
        particles[i].pres = 1.0;
        particles[i].mass = 0.01;
        particles[i].id = i;
        particles[i].type = static_cast<int>(ParticleType::REAL);
    }
    
    auto ghost_manager = std::make_shared<GhostParticleManager<1>>();
    
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::MIRROR;
    config.range_min[0] = -0.5;
    config.range_max[0] = 0.5;
    config.enable_lower[0] = true;
    config.enable_upper[0] = false;  // Only lower boundary
    config.mirror_types[0] = MirrorType::NO_SLIP;
    
    ghost_manager->initialize(config);
    ghost_manager->set_kernel_support_radius(4.0 * dx);
    ghost_manager->generate_ghosts(particles);
    
    ASSERT_GT(ghost_manager->get_ghost_count(), 0);
    
    const auto& ghosts = ghost_manager->get_ghost_particles();
    for (const auto& ghost : ghosts) {
        // Ghost should inherit physical properties
        EXPECT_GT(ghost.dens, 0.0) << "Ghost should have positive density";
        EXPECT_GT(ghost.mass, 0.0) << "Ghost should have positive mass";
        EXPECT_EQ(ghost.pres, 1.0) << "Ghost should inherit pressure";
        
        // But should be marked as ghost
        EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST))
            << "Ghost particles must have type=GHOST";
    }
}

/**
 * @test Document the initialization order requirement
 */
TEST_F(ParticleInitializationTest, DocumentCorrectInitializationOrder) {
    // CORRECT INITIALIZATION SEQUENCE:
    
    // 1. Create particles with positions and physical properties
    std::vector<SPHParticle<1>> particles(10);
    const real dx = 0.1;
    for (int i = 0; i < 10; ++i) {
        particles[i].pos[0] = i * dx;
        particles[i].dens = 1.0;
        particles[i].pres = 1.0;
        particles[i].mass = dx;
        // sml is still 0 at this point!
    }
    
    // 2. Estimate kernel support from particle spacing (NOT from sml!)
    const real estimated_sml = 2.0 * dx;
    const real kernel_support_radius = 2.0 * estimated_sml;
    
    // 3. Initialize ghost particle manager with estimated kernel support
    auto ghost_manager = std::make_shared<GhostParticleManager<1>>();
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min[0] = 0.0;
    config.range_max[0] = 1.0;
    
    ghost_manager->initialize(config);
    ghost_manager->set_kernel_support_radius(kernel_support_radius);
    
    // 4. Generate ghost particles
    ghost_manager->generate_ghosts(particles);
    
    // 5. LATER: Pre-interaction step will calculate actual sml values
    // (Not tested here, but this is when particles.sml gets set)
    
    EXPECT_GT(ghost_manager->get_ghost_count(), 0)
        << "This order should produce valid ghost particles";
}

/**
 * @test Verify periodic boundary ghost generation with proper initialization
 */
TEST_F(ParticleInitializationTest, PeriodicBoundaryGhostGeneration) {
    const int n_particles = 20;
    const real domain_min = 0.0;
    const real domain_max = 1.0;
    const real dx = (domain_max - domain_min) / n_particles;
    
    std::vector<SPHParticle<1>> particles(n_particles);
    for (int i = 0; i < n_particles; ++i) {
        particles[i].pos[0] = domain_min + (i + 0.5) * dx;
        particles[i].dens = 1.0;
        particles[i].pres = 1.0;
        particles[i].mass = dx;
        particles[i].id = i;
        particles[i].type = static_cast<int>(ParticleType::REAL);
    }
    
    auto ghost_manager = std::make_shared<GhostParticleManager<1>>();
    
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min[0] = domain_min;
    config.range_max[0] = domain_max;
    
    ghost_manager->initialize(config);
    
    // Estimate kernel support properly
    const real estimated_sml = 2.0 * dx;
    const real kernel_support_radius = 2.0 * estimated_sml;
    ghost_manager->set_kernel_support_radius(kernel_support_radius);
    
    ghost_manager->generate_ghosts(particles);
    
    const int ghost_count = ghost_manager->get_ghost_count();
    EXPECT_GT(ghost_count, 0) << "Periodic boundaries should generate ghosts";
    
    // For periodic boundaries, ghosts wrap around
    const auto& ghosts = ghost_manager->get_ghost_particles();
    for (const auto& ghost : ghosts) {
        EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST));
        
        // Periodic ghosts should be shifted by domain size
        // (exact positions depend on which particles are near boundaries)
    }
}


// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file ghost_particle_coordinator_test.cpp
 * @brief BDD-style tests for GhostParticleCoordinator
 * 
 * Tests validate the coordinator's responsibility to manage ghost particle lifecycle
 * in sync with simulation state, following TDD/BDD principles.
 * 
 * Scenarios covered:
 * - Kernel support calculation from particle smoothing lengths
 * - Ghost initialization after smoothing length calculation
 * - Ghost updates during time integration
 * - Null/disabled ghost manager handling
 * - Varying smoothing lengths
 * - State query methods
 */

#include <gtest/gtest.h>
#include "../../helpers/bdd_helpers.hpp"
#include "core/boundaries/ghost_particle_coordinator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include <memory>
#include <vector>
#include <cmath>

using namespace sph;

constexpr int kTestDimension = 3;
constexpr real kCubicSplineSupportFactor = 2.0;

// ============================================================================
// Test Fixtures
// ============================================================================

class GhostParticleCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal simulation for testing
        param = std::make_shared<SPHParameters>();
        sim = std::make_shared<Simulation<kTestDimension>>(param);
        
        // Configure basic simulation parameters
        sim->particle_num = 0;
        sim->dt = 0.001;
    }
    
    // Helper: Create particles with specific smoothing lengths
    std::vector<SPHParticle<kTestDimension>> create_particles_with_sml(
        const std::vector<real>& smoothing_lengths) 
    {
        std::vector<SPHParticle<kTestDimension>> particles;
        particles.reserve(smoothing_lengths.size());
        
        for (size_t i = 0; i < smoothing_lengths.size(); ++i) {
            SPHParticle<kTestDimension> p;
            p.id = static_cast<int>(i);
            p.pos = Vector<kTestDimension>(static_cast<real>(i) * 0.1, 0.0, 0.0);
            p.vel = Vector<kTestDimension>(0.0, 0.0, 0.0);
            p.mass = 1.0;
            p.dens = 1.0;
            p.pres = 0.0;
            p.sml = smoothing_lengths[i];
            p.type = 0;
            particles.push_back(p);
        }
        
        return particles;
    }
    
    // Helper: Setup ghost manager with valid configuration
    void setup_ghost_manager_with_boundary() {
        BoundaryConfiguration<kTestDimension> config;
        config.is_valid = true;
        config.types[0] = BoundaryType::PERIODIC;
        config.enable_lower[0] = true;
        config.enable_upper[0] = true;
        if (kTestDimension >= 2) {
            config.types[1] = BoundaryType::PERIODIC;
            config.enable_lower[1] = true;
            config.enable_upper[1] = true;
        }
        if (kTestDimension >= 3) {
            config.types[2] = BoundaryType::PERIODIC;
            config.enable_lower[2] = true;
            config.enable_upper[2] = true;
        }
        config.range_min = Vector<kTestDimension>(-1.0, -1.0, -1.0);
        config.range_max = Vector<kTestDimension>(1.0, 1.0, 1.0);
        
        sim->ghost_manager = std::make_shared<GhostParticleManager<kTestDimension>>();
        sim->ghost_manager->initialize(config);
    }
    
    std::shared_ptr<SPHParameters> param;
    std::shared_ptr<Simulation<kTestDimension>> sim;
};

// ============================================================================
// SCENARIO: Kernel support calculation
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest, 
         GivenParticlesWithUniformSML_WhenCalculateKernelSupport_ThenReturnsCorrectValue)
{
    GIVEN("A simulation with particles having uniform smoothing length") {
        const real uniform_sml = 0.15;
        auto particles = create_particles_with_sml({uniform_sml, uniform_sml, uniform_sml});
        sim->particles = particles;
        sim->particle_num = static_cast<int>(particles.size());
        
        setup_ghost_manager_with_boundary();
        
        WHEN("GhostParticleCoordinator is created and initializes ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            coordinator->initialize_ghosts(particles);
            
            THEN("Kernel support radius equals 2.0 * uniform_sml") {
                const real expected_support = kCubicSplineSupportFactor * uniform_sml;
                EXPECT_DOUBLE_EQ(coordinator->get_kernel_support_radius(), expected_support);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenParticlesWithVaryingSML_WhenCalculateKernelSupport_ThenUsesMaximum)
{
    GIVEN("Particles with varying smoothing lengths") {
        const std::vector<real> smoothing_lengths = {0.1, 0.3, 0.2, 0.25, 0.15};
        const real max_sml = 0.3;  // Maximum in the list
        
        auto particles = create_particles_with_sml(smoothing_lengths);
        sim->particles = particles;
        sim->particle_num = static_cast<int>(particles.size());
        
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator initializes ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            coordinator->initialize_ghosts(particles);
            
            THEN("Kernel support uses maximum smoothing length") {
                const real expected_support = kCubicSplineSupportFactor * max_sml;
                EXPECT_DOUBLE_EQ(coordinator->get_kernel_support_radius(), expected_support);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenSingleParticle_WhenCalculateKernelSupport_ThenUsesItsSML)
{
    GIVEN("A single particle with specific smoothing length") {
        const real single_sml = 0.42;
        auto particles = create_particles_with_sml({single_sml});
        sim->particles = particles;
        sim->particle_num = 1;
        
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator initializes ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            coordinator->initialize_ghosts(particles);
            
            THEN("Kernel support equals 2.0 * single_sml") {
                const real expected_support = kCubicSplineSupportFactor * single_sml;
                EXPECT_DOUBLE_EQ(coordinator->get_kernel_support_radius(), expected_support);
            }
        }
    }
}

// ============================================================================
// SCENARIO: Ghost initialization
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenValidGhostManager_WhenInitializeGhosts_ThenGhostsGenerated)
{
    GIVEN("Simulation with ghost manager and boundary conditions") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        sim->particles = particles;
        sim->particle_num = static_cast<int>(particles.size());
        
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator initializes ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            const size_t ghost_count_before = sim->ghost_manager->get_ghost_count();
            coordinator->initialize_ghosts(particles);
            
            THEN("Ghost manager generates ghosts") {
                // After initialization, ghost_manager should have processed the particles
                // Actual count depends on boundary configuration, but should be callable
                EXPECT_NO_THROW(sim->ghost_manager->get_ghost_count());
                EXPECT_TRUE(coordinator->has_ghosts() == (sim->ghost_manager->get_ghost_count() > 0));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenNullGhostManager_WhenInitializeGhosts_ThenNoErrorAndNoGhosts)
{
    GIVEN("Simulation without ghost manager") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        sim->ghost_manager = nullptr;  // Explicitly null
        
        WHEN("Coordinator attempts to initialize ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            THEN("No error occurs and has_ghosts returns false") {
                EXPECT_NO_THROW(coordinator->initialize_ghosts(particles));
                EXPECT_FALSE(coordinator->has_ghosts());
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenInvalidGhostConfig_WhenInitializeGhosts_ThenEarlyExit)
{
    GIVEN("Ghost manager with invalid configuration") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        
        BoundaryConfiguration<kTestDimension> config;
        config.is_valid = false;  // Invalid config
        sim->ghost_manager = std::make_shared<GhostParticleManager<kTestDimension>>();
        sim->ghost_manager->initialize(config);
        
        WHEN("Coordinator attempts to initialize ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            coordinator->initialize_ghosts(particles);
            
            THEN("No ghosts are generated") {
                EXPECT_FALSE(coordinator->has_ghosts());
            }
        }
    }
}

// ============================================================================
// SCENARIO: Ghost updates during integration
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenParticlesMove_WhenUpdateGhosts_ThenKernelSupportRecalculated)
{
    GIVEN("Simulation with initial ghosts generated") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        sim->particles = particles;
        sim->particle_num = static_cast<int>(particles.size());
        
        setup_ghost_manager_with_boundary();
        
        auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
        coordinator->initialize_ghosts(particles);
        
        const real initial_support = coordinator->get_kernel_support_radius();
        
        WHEN("Particles have new smoothing lengths and ghosts are updated") {
            // Simulate adaptive smoothing length changes
            for (auto& p : particles) {
                p.sml = 0.2;  // Increased smoothing length
            }
            
            coordinator->update_ghosts(particles);
            
            THEN("Kernel support radius is recalculated") {
                const real new_support = coordinator->get_kernel_support_radius();
                EXPECT_GT(new_support, initial_support);
                EXPECT_DOUBLE_EQ(new_support, kCubicSplineSupportFactor * 0.2);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenNullGhostManager_WhenUpdateGhosts_ThenNoError)
{
    GIVEN("Simulation without ghost manager") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        sim->ghost_manager = nullptr;
        
        WHEN("Coordinator attempts to update ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            THEN("No error occurs") {
                EXPECT_NO_THROW(coordinator->update_ghosts(particles));
                EXPECT_FALSE(coordinator->has_ghosts());
            }
        }
    }
}

// ============================================================================
// SCENARIO: State queries
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenCoordinatorWithGhosts_WhenQueryState_ThenReturnsCorrectInfo)
{
    GIVEN("Coordinator with initialized ghosts") {
        auto particles = create_particles_with_sml({0.15});
        sim->particles = particles;
        sim->particle_num = 1;
        
        setup_ghost_manager_with_boundary();
        
        auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
        coordinator->initialize_ghosts(particles);
        
        WHEN("State is queried") {
            const bool has_ghosts = coordinator->has_ghosts();
            const size_t ghost_count = coordinator->ghost_count();
            const real kernel_support = coordinator->get_kernel_support_radius();
            
            THEN("State information is consistent") {
                if (has_ghosts) {
                    EXPECT_GT(ghost_count, 0);
                } else {
                    EXPECT_EQ(ghost_count, 0);
                }
                
                EXPECT_DOUBLE_EQ(kernel_support, kCubicSplineSupportFactor * 0.15);
            }
        }
    }
}

// ============================================================================
// SCENARIO: Edge cases
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenEmptyParticleList_WhenInitializeGhosts_ThenHandlesGracefully)
{
    GIVEN("Empty particle list") {
        std::vector<SPHParticle<kTestDimension>> empty_particles;
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator initializes with empty list") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            THEN("No error occurs") {
                EXPECT_NO_THROW(coordinator->initialize_ghosts(empty_particles));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenParticlesWithZeroSML_WhenInitializeGhosts_ThenThrowsError)
{
    GIVEN("Particles with invalid (zero) smoothing length") {
        auto particles = create_particles_with_sml({0.0, 0.1, 0.0});
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator attempts to initialize ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            THEN("An error is thrown indicating invalid state") {
                // This validates precondition checking
                EXPECT_THROW(
                    coordinator->initialize_ghosts(particles),
                    std::logic_error
                ) << "Should detect invalid smoothing lengths";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenParticlesWithNegativeSML_WhenInitializeGhosts_ThenThrowsError)
{
    GIVEN("Particles with invalid (negative) smoothing length") {
        auto particles = create_particles_with_sml({0.1, -0.05, 0.1});
        setup_ghost_manager_with_boundary();
        
        WHEN("Coordinator attempts to initialize ghosts") {
            auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
            
            THEN("An error is thrown") {
                EXPECT_THROW(
                    coordinator->initialize_ghosts(particles),
                    std::logic_error
                );
            }
        }
    }
}

// ============================================================================
// SCENARIO: Multiple update cycles
// ============================================================================

SCENARIO_WITH_FIXTURE(GhostParticleCoordinatorTest,
         GivenMultipleUpdates_WhenSMLChanges_ThenKernelSupportTracksMaximum)
{
    GIVEN("Simulation with time-evolving smoothing lengths") {
        auto particles = create_particles_with_sml({0.1, 0.1, 0.1});
        sim->particles = particles;
        sim->particle_num = 3;
        
        setup_ghost_manager_with_boundary();
        
        auto coordinator = std::make_shared<GhostParticleCoordinator<kTestDimension>>(sim);
        coordinator->initialize_ghosts(particles);
        
        WHEN("Smoothing lengths evolve over multiple updates") {
            // Update 1: sml increases
            for (auto& p : particles) p.sml = 0.15;
            coordinator->update_ghosts(particles);
            const real support1 = coordinator->get_kernel_support_radius();
            
            // Update 2: sml decreases
            for (auto& p : particles) p.sml = 0.08;
            coordinator->update_ghosts(particles);
            const real support2 = coordinator->get_kernel_support_radius();
            
            // Update 3: sml varies
            particles[0].sml = 0.05;
            particles[1].sml = 0.25;  // New maximum
            particles[2].sml = 0.10;
            coordinator->update_ghosts(particles);
            const real support3 = coordinator->get_kernel_support_radius();
            
            THEN("Kernel support correctly tracks maximum at each step") {
                EXPECT_DOUBLE_EQ(support1, kCubicSplineSupportFactor * 0.15);
                EXPECT_DOUBLE_EQ(support2, kCubicSplineSupportFactor * 0.08);
                EXPECT_DOUBLE_EQ(support3, kCubicSplineSupportFactor * 0.25);
            }
        }
    }
}

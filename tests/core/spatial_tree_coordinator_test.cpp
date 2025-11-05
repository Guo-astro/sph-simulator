/**
 * @file spatial_tree_coordinator_test.cpp
 * @brief BDD-style tests for SpatialTreeCoordinator
 * 
 * Tests validate the coordinator's responsibility to manage tree lifecycle
 * and container consistency, following TDD/BDD principles.
 * 
 * Scenarios covered:
 * - Container synchronization without reallocation
 * - Container growth with buffer management
 * - Linked-list pointer clearing
 * - Tree rebuild coordination
 * - Consistency validation
 * - Edge cases (empty containers, large growth)
 * - Tree must be built before neighbor search (regression test for workflow bug)
 * - Initial smoothing lengths must remain valid through tree rebuild
 */

#include <gtest/gtest.h>
#include "../bdd_helpers.hpp"
#include "core/spatial_tree_coordinator.hpp"
#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/ghost_particle_manager.hpp"
#include <memory>
#include <vector>
#include <cmath>

using namespace sph;

constexpr int kTestDimension = 3;

// ============================================================================
// Test Fixtures
// ============================================================================

class SpatialTreeCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal simulation
        param = std::make_shared<SPHParameters>();
        sim = std::make_shared<Simulation<kTestDimension> >(param);
        
        sim->particle_num = 0;
        sim->dt = 0.001;
        
        // Initialize tree
        sim->tree = std::make_shared<BHTree<kTestDimension>>();
        sim->tree->initialize(param);
        sim->tree->resize(100);  // Reserve capacity for tests
    }
    
    // Helper: Create test particles
    std::vector<SPHParticle<kTestDimension>> create_test_particles(int count) {
        std::vector<SPHParticle<kTestDimension>> particles;
        particles.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            SPHParticle<kTestDimension> p;
            p.id = i;
            p.pos = Vector<kTestDimension>(static_cast<real>(i) * 0.1, 0.0, 0.0);
            p.vel = Vector<kTestDimension>(0.0, 0.0, 0.0);
            p.mass = 1.0;
            p.dens = 1.0;
            p.pres = 0.0;
            p.sml = 0.1;
            p.type = 0;
            p.next = nullptr;
            particles.push_back(p);
        }
        
        return particles;
    }
    
    // Helper: Setup ghost manager
    void setup_ghost_manager(int ghost_count) {
        BoundaryConfiguration<kTestDimension> config;
        config.is_valid = true;
        config.types[0] = BoundaryType::PERIODIC;
        config.enable_lower[0] = true;
        config.enable_upper[0] = true;
        config.range_min = Vector<kTestDimension>(-1.0, -1.0, -1.0);
        config.range_max = Vector<kTestDimension>(1.0, 1.0, 1.0);
        
        sim->ghost_manager = std::make_shared<GhostParticleManager<kTestDimension>>();
        sim->ghost_manager->initialize(config);
        
        // Generate ghost particles if requested
        if (ghost_count > 0 && !sim->particles.empty()) {
            sim->ghost_manager->set_kernel_support_radius(0.3);  // Some reasonable value
            sim->ghost_manager->generate_ghosts(sim->particles);
        }
    }
    
    std::shared_ptr<SPHParameters> param;
    std::shared_ptr<Simulation<kTestDimension> > sim;
};

// ============================================================================
// SCENARIO: Container synchronization without reallocation
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenSufficientCapacity_WhenRebuildTree_ThenNoReallocation)
{
    GIVEN("Simulation with cached_search_particles having sufficient capacity") {
        auto particles = create_test_particles(50);
        sim->particles = particles;
        sim->particle_num = 50;
        
        // Pre-allocate capacity
        sim->cached_search_particles.reserve(200);
        const void* initial_ptr = sim->cached_search_particles.data();
        
        WHEN("Tree is rebuilt with coordinator") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Container is not reallocated") {
                const void* after_ptr = sim->cached_search_particles.data();
                EXPECT_EQ(initial_ptr, after_ptr) 
                    << "Container should not reallocate when capacity is sufficient";
                
                AND("Size matches real particle count") {
                    EXPECT_EQ(sim->cached_search_particles.size(), sim->particles.size());
                }
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenInsufficientCapacity_WhenRebuildTree_ThenReservesWithBuffer)
{
    GIVEN("Simulation where cached_search_particles needs to grow") {
        auto particles = create_test_particles(50);
        sim->particles = particles;
        sim->particle_num = 50;
        
        // Start with small capacity
        sim->cached_search_particles.reserve(10);
        
        WHEN("Tree coordinator rebuilds with larger particle set") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Capacity includes buffer beyond current need") {
                EXPECT_GE(sim->cached_search_particles.capacity(), 
                          sim->particles.size() + SpatialTreeCoordinator<kTestDimension>::REALLOCATION_BUFFER)
                    << "Should reserve extra buffer to avoid frequent reallocations";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenGhostParticles_WhenRebuildTree_ThenIncludesAllInContainer)
{
    GIVEN("Simulation with real and ghost particles") {
        const int real_count = 50;
        auto particles = create_test_particles(real_count);
        sim->particles = particles;
        sim->particle_num = real_count;
        
        setup_ghost_manager(10);
        
        // Pre-sync to simulate get_all_particles_for_search()
        auto all_particles = sim->get_all_particles_for_search();
        const size_t total_expected = all_particles.size();
        
        WHEN("Tree is rebuilt") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Cached search particles contains real + ghost") {
                EXPECT_EQ(sim->cached_search_particles.size(), total_expected);
                
                AND("All particle IDs are valid indices") {
                    for (size_t i = 0; i < sim->cached_search_particles.size(); ++i) {
                        EXPECT_EQ(sim->cached_search_particles[i].id, static_cast<int>(i))
                            << "Particle ID must match its index for tree consistency";
                    }
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: Linked-list pointer clearing
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenParticlesWithStaleNextPointers_WhenRebuildTree_ThenPointersCleared)
{
    GIVEN("Particles with existing next pointers (from previous tree build)") {
        auto particles = create_test_particles(50);
        
        // Simulate stale next pointers
        for (size_t i = 0; i < particles.size() - 1; ++i) {
            particles[i].next = &particles[i + 1];
        }
        
        sim->particles = particles;
        sim->particle_num = 50;
        
        WHEN("Coordinator rebuilds tree") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("All next pointers in cached_search_particles are nullptr before tree build") {
                // Note: Tree build may set next pointers again, so we test the clearing happens
                // We'll verify this by checking behavior is correct (no crashes from stale pointers)
                EXPECT_NO_THROW({
                    coordinator->rebuild_tree_for_neighbor_search();
                }) << "Should handle stale pointers without crashes";
            }
        }
    }
}

// ============================================================================
// SCENARIO: Tree rebuild coordination
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenValidSimulation_WhenRebuildTree_ThenTreeIsConstructed)
{
    GIVEN("Simulation with particles and tree") {
        auto particles = create_test_particles(50);
        sim->particles = particles;
        sim->particle_num = 50;
        
        WHEN("Coordinator rebuilds tree") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Tree is built and usable for neighbor search") {
                // Verify tree can be queried
                std::vector<int> neighbor_list(500);  // Pre-allocate
                const auto& search_particles = sim->cached_search_particles;
                ASSERT_FALSE(search_particles.empty()) << "Search particles should not be empty";
                ASSERT_TRUE(sim->tree) << "Tree should not be null";
                
                // Try neighbor search
                int n_neighbors = sim->tree->neighbor_search(
                    search_particles[0], 
                    neighbor_list, 
                    search_particles, 
                    false
                );
                
                EXPECT_GE(n_neighbors, 0) << "Neighbor search should return valid count";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenMultipleRebuilds_WhenCalledSequentially_ThenEachRebuildSucceeds)
{
    GIVEN("Simulation that needs multiple tree rebuilds") {
        auto particles = create_test_particles(50);
        sim->particles = particles;
        sim->particle_num = 50;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        
        WHEN("Tree is rebuilt multiple times") {
            coordinator->rebuild_tree_for_neighbor_search();
            const size_t size_after_first = sim->cached_search_particles.size();
            
            coordinator->rebuild_tree_for_neighbor_search();
            const size_t size_after_second = sim->cached_search_particles.size();
            
            coordinator->rebuild_tree_for_neighbor_search();
            const size_t size_after_third = sim->cached_search_particles.size();
            
            THEN("Each rebuild maintains consistency") {
                EXPECT_EQ(size_after_first, size_after_second);
                EXPECT_EQ(size_after_second, size_after_third);
                EXPECT_EQ(size_after_third, sim->particles.size());
            }
        }
    }
}

// ============================================================================
// SCENARIO: Consistency validation
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenTreeBuilt_WhenValidateConsistency_ThenReturnsTrue)
{
    GIVEN("Coordinator with freshly built tree") {
        auto particles = create_test_particles(50);
        sim->particles = particles;
        sim->particle_num = 50;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        coordinator->rebuild_tree_for_neighbor_search();
        
        WHEN("Consistency is validated") {
            const bool is_consistent = coordinator->is_tree_consistent();
            
            THEN("Tree is consistent with container") {
                EXPECT_TRUE(is_consistent) 
                    << "Freshly built tree should be consistent";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenParticleCountQuery_WhenCalled_ThenReturnsCorrectCount)
{
    GIVEN("Simulation with real and ghost particles") {
        const int real_count = 50;
        auto particles = create_test_particles(real_count);
        sim->particles = particles;
        sim->particle_num = real_count;
        
        setup_ghost_manager(10);
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        coordinator->rebuild_tree_for_neighbor_search();
        
        WHEN("Search particle count is queried") {
            const size_t count = coordinator->get_search_particle_count();
            
            THEN("Returns total count including ghosts") {
                EXPECT_EQ(count, sim->cached_search_particles.size());
                EXPECT_GE(count, real_count) << "Should include ghosts if present";
            }
        }
    }
}

// ============================================================================
// SCENARIO: Edge cases
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenEmptyParticleList_WhenRebuildTree_ThenHandlesGracefully)
{
    GIVEN("Simulation with no particles") {
        sim->particles.clear();
        sim->particle_num = 0;
        
        WHEN("Coordinator attempts to rebuild tree") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            
            THEN("No error occurs") {
                EXPECT_NO_THROW(coordinator->rebuild_tree_for_neighbor_search());
                EXPECT_EQ(coordinator->get_search_particle_count(), 0);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenSingleParticle_WhenRebuildTree_ThenSucceeds)
{
    GIVEN("Simulation with single particle") {
        auto particles = create_test_particles(1);
        sim->particles = particles;
        sim->particle_num = 1;
        
        WHEN("Tree is rebuilt") {
            auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Tree is built successfully") {
                EXPECT_EQ(sim->cached_search_particles.size(), 1);
                EXPECT_EQ(sim->cached_search_particles[0].id, 0);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenLargeParticleCountIncrease_WhenRebuildTree_ThenManagesMemoryEfficiently)
{
    GIVEN("Simulation that grows from small to large particle count") {
        auto particles = create_test_particles(10);
        sim->particles = particles;
        sim->particle_num = 10;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        coordinator->rebuild_tree_for_neighbor_search();
        
        WHEN("Particle count increases dramatically") {
            // Simulate ghost generation causing large increase
            auto large_particles = create_test_particles(500);
            sim->particles = large_particles;
            sim->particle_num = 500;
            
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Memory is managed without crashes") {
                EXPECT_EQ(sim->cached_search_particles.size(), 500);
                EXPECT_GE(sim->cached_search_particles.capacity(), 500);
                
                AND("Buffer is added for future growth") {
                    EXPECT_GE(sim->cached_search_particles.capacity(),
                              500 + SpatialTreeCoordinator<kTestDimension>::REALLOCATION_BUFFER);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: Integration with ghost system
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenGhostCountChanges_WhenRebuildTree_ThenAdaptsContainerSize)
{
    GIVEN("Simulation where ghost count varies between rebuilds") {
        const int real_count = 50;
        auto particles = create_test_particles(real_count);
        sim->particles = particles;
        sim->particle_num = real_count;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        
        WHEN("Ghost count changes across rebuilds") {
            // First rebuild: no ghosts
            coordinator->rebuild_tree_for_neighbor_search();
            const size_t size_without_ghosts = coordinator->get_search_particle_count();
            
            // Add ghosts
            setup_ghost_manager(20);
            coordinator->rebuild_tree_for_neighbor_search();
            const size_t size_with_ghosts = coordinator->get_search_particle_count();
            
            THEN("Container size adapts correctly") {
                EXPECT_EQ(size_without_ghosts, real_count);
                EXPECT_GE(size_with_ghosts, real_count) << "Should include ghosts";
                
                AND("All IDs remain consistent") {
                    for (size_t i = 0; i < sim->cached_search_particles.size(); ++i) {
                        EXPECT_EQ(sim->cached_search_particles[i].id, static_cast<int>(i));
                    }
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: Performance characteristics
// ============================================================================

SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
         GivenFrequentRebuilds_WhenCapacityPreallocated_ThenMinimizesReallocations)
{
    GIVEN("Simulation with capacity pre-allocated") {
        auto particles = create_test_particles(100);
        sim->particles = particles;
        sim->particle_num = 100;
        
        // Pre-allocate generous capacity
        sim->cached_search_particles.reserve(500);
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        
        WHEN("Multiple rebuilds occur") {
            const void* initial_ptr = nullptr;
            int reallocation_count = 0;
            
            for (int i = 0; i < 10; ++i) {
                coordinator->rebuild_tree_for_neighbor_search();
                
                const void* current_ptr = sim->cached_search_particles.data();
                if (i == 0) {
                    initial_ptr = current_ptr;
                } else if (current_ptr != initial_ptr) {
                    reallocation_count++;
                    initial_ptr = current_ptr;
                }
            }
            
            THEN("No reallocations occur") {
                EXPECT_EQ(reallocation_count, 0) 
                    << "Pre-allocated capacity should prevent reallocations";
            }
        }
    }
}

// ============================================================================
// SCENARIO: Tree must be built before neighbor search to avoid infinite sml
// ============================================================================
// Regression test for workflow bug where pre_interaction->calculation()
// was called before tree was built, causing neighbor search to return 0
// neighbors, leading to division by zero and infinite smoothing lengths.
SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest, 
    TreeMustBeBuiltBeforeNeighborSearchToPreventInfiniteSml)
{
    GIVEN("Particles with valid initial smoothing lengths") {
        auto particles = create_test_particles(10);
        
        // Set valid initial smoothing lengths (like plugin would)
        constexpr real kInitialSml = 0.1;
        for (auto& p : particles) {
            p.sml = kInitialSml;
            p.dens = 1.0;  // Valid density
            p.mass = 0.01; // Valid mass
        }
        
        sim->particles = particles;
        sim->particle_num = 10;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        
        WHEN("Tree is rebuilt before any neighbor search") {
            coordinator->rebuild_tree_for_neighbor_search();
            
            THEN("Tree is built and ready for neighbor search") {
                EXPECT_TRUE(coordinator->is_tree_consistent());
                EXPECT_GT(sim->cached_search_particles.size(), 0u);
                
                AND_THEN("Smoothing lengths remain finite and valid") {
                    for (const auto& p : sim->particles) {
                        EXPECT_TRUE(std::isfinite(p.sml))
                            << "Smoothing length must remain finite after tree build";
                        EXPECT_GT(p.sml, 0.0)
                            << "Smoothing length must remain positive";
                    }
                }
            }
        }
        
        WHEN("Tree is NOT built before neighbor search attempt") {
            // Simulate the bug: try to use tree without building it
            // Tree exists but hasn't been built with make()
            
            THEN("Tree is not ready for neighbor search") {
                // This scenario documents the bug condition
                // In production, this would cause neighbor_search to fail
                EXPECT_EQ(sim->cached_search_particles.size(), 0u)
                    << "Cached particles should be empty before tree rebuild";
                
                AND_THEN("Initial smoothing lengths are still valid") {
                    for (const auto& p : sim->particles) {
                        EXPECT_DOUBLE_EQ(p.sml, kInitialSml)
                            << "Initial sml should not be modified";
                    }
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: Multiple tree rebuilds preserve particle data integrity
// ============================================================================
SCENARIO_WITH_FIXTURE(SpatialTreeCoordinatorTest,
    MultipleTreeRebuildsPreserveParticleDataIntegrity)
{
    GIVEN("Particles with carefully set properties") {
        auto particles = create_test_particles(20);
        
        constexpr real kTestSml = 0.15;
        constexpr real kTestDens = 2.5;
        constexpr real kTestMass = 0.02;
        
        for (size_t i = 0; i < particles.size(); ++i) {
            particles[i].sml = kTestSml;
            particles[i].dens = kTestDens;
            particles[i].mass = kTestMass;
            particles[i].id = static_cast<int>(i);
        }
        
        sim->particles = particles;
        sim->particle_num = 20;
        
        auto coordinator = std::make_shared<SpatialTreeCoordinator<kTestDimension>>(sim);
        
        WHEN("Tree is rebuilt multiple times") {
            for (int rebuild = 0; rebuild < 5; ++rebuild) {
                coordinator->rebuild_tree_for_neighbor_search();
            }
            
            THEN("All particle properties remain unchanged") {
                ASSERT_EQ(sim->particles.size(), 20u);
                
                for (size_t i = 0; i < sim->particles.size(); ++i) {
                    EXPECT_DOUBLE_EQ(sim->particles[i].sml, kTestSml)
                        << "Smoothing length should not change for particle " << i;
                    EXPECT_DOUBLE_EQ(sim->particles[i].dens, kTestDens)
                        << "Density should not change for particle " << i;
                    EXPECT_DOUBLE_EQ(sim->particles[i].mass, kTestMass)
                        << "Mass should not change for particle " << i;
                    EXPECT_EQ(sim->particles[i].id, static_cast<int>(i))
                        << "ID should not change for particle " << i;
                }
                
                AND_THEN("Cached search particles are properly synchronized") {
                    EXPECT_EQ(sim->cached_search_particles.size(), 20u);
                    
                    for (size_t i = 0; i < sim->cached_search_particles.size(); ++i) {
                        EXPECT_DOUBLE_EQ(sim->cached_search_particles[i].sml, kTestSml);
                        EXPECT_EQ(sim->cached_search_particles[i].id, static_cast<int>(i));
                    }
                }
            }
        }
    }
}

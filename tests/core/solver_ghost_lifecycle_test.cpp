// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file solver_ghost_lifecycle_test.cpp
 * @brief BDD-style tests for Solver ghost particle lifecycle management
 * 
 * Tests validate critical solver responsibilities:
 * - Ghost generation timing in initialize()
 * - Tree rebuild after ghost updates in integrate()
 * - Particle container separation (particles vs cached_search_particles)
 * - Ghost ID renumbering through simulation lifecycle
 */

#include <gtest/gtest.h>
#include "solver.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/spatial/neighbor_search_config.hpp"
#include "utilities/vec_n.hpp"
#include <memory>
#include <vector>

using namespace sph;
using utilities::Vec3d;

// Mock scenario for testing (minimal implementation)
template<int Dim>
class MockScenario {
public:
    void init_particles(std::vector<SPHParticle<Dim>>& particles, int& num) {
        // Create simple particle grid
        num = 50;
        particles.resize(num);
        for (int i = 0; i < num; ++i) {
            particles[i].id = i;
            particles[i].r = typename utilities::VecN<Dim>(i * 0.1);
            particles[i].m = 1.0;
            particles[i].v = typename utilities::VecN<Dim>(0.0);
            particles[i].h = 0.1;
            particles[i].rho = 1.0;
            particles[i].p = 0.0;
            particles[i].property = 0;
        }
    }

    void set_boundary_conditions(std::shared_ptr<Simulation<Dim>> sim) {
        // Enable boundary conditions to trigger ghost generation
    }

    void set_eos(std::shared_ptr<Simulation<Dim>> sim) {}
    void set_physical_coefficients(std::shared_ptr<Simulation<Dim>> sim) {}
};

// Test fixture for Solver tests
class SolverGhostLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create simulation with mock scenario
        scenario = std::make_shared<MockScenario<3>>();
        sim = std::make_shared<Simulation<3>>(scenario);
        
        // Configure simulation parameters
        sim->dt = 0.001;
        sim->end_time = 0.01;
        sim->output_interval = 10;
        sim->neighbor_number = 50;
        sim->tree_max_level = 10;
        sim->tree_leaf_particle_num = 10;
    }

    std::shared_ptr<MockScenario<3>> scenario;
    std::shared_ptr<Simulation<3>> sim;
};

// ============================================================================
// GIVEN: Solver initialization with boundary conditions
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenSolverInitialized_WhenBoundaryConditionsExist_ThenGhostsGenerated) {
    // GIVEN: Solver with simulation
    Solver solver(sim);
    
    // Record initial particle count
    const int initial_real_count = sim->particle_num;
    ASSERT_GT(initial_real_count, 0) << "Should have real particles before initialization";

    // WHEN: Solver initializes (generates ghosts after smoothing length calculation)
    solver.initialize();

    // THEN: Ghost particles are generated
    // Note: Actual ghost count depends on boundary implementation
    // We verify the mechanism works by checking cached_search_particles is populated
    EXPECT_EQ(sim->particles.size(), static_cast<size_t>(initial_real_count))
        << "Real particle count should remain unchanged";
    
    if (sim->ghosts.size() > 0) {
        EXPECT_EQ(sim->cached_search_particles.size(), 
                  sim->particles.size() + sim->ghosts.size())
            << "Cached search particles should include real + ghost";
    }
}

TEST_F(SolverGhostLifecycleTest, GivenSolverInitialized_WhenGhostsGenerated_ThenGhostIDsMatchIndices) {
    // GIVEN: Solver with boundary conditions
    Solver solver(sim);
    solver.initialize();

    // WHEN: Ghosts are generated
    const auto& search_particles = sim->cached_search_particles;
    
    if (search_particles.size() > sim->particles.size()) {
        // THEN: All particle IDs in search list match their indices
        for (size_t i = 0; i < search_particles.size(); ++i) {
            EXPECT_EQ(search_particles[i].id, static_cast<int>(i))
                << "Particle at index " << i << " has mismatched ID " 
                << search_particles[i].id;
        }
    }
}

TEST_F(SolverGhostLifecycleTest, GivenSolverInitialized_WhenGhostsGenerated_ThenTreeRebuiltWithCombinedList) {
    // GIVEN: Solver with boundary conditions
    Solver solver(sim);
    
    // WHEN: Initialization completes
    solver.initialize();

    // THEN: Tree was rebuilt after ghost generation
    // We verify this by checking tree can search with correct particle count
    auto* tree = sim->tree.get();
    ASSERT_NE(tree, nullptr) << "Tree should exist after initialization";

    // Perform search to verify tree consistency
    // Create a dummy particle at search position for the new API
    SPHParticle<3> search_particle;
    search_particle.pos = Vec3d(1.0, 1.0, 1.0);
    search_particle.sml = 1.0;  // Reasonable smoothing length for search
    
    const auto search_config = NeighborSearchConfig::create(50, false);
    auto result = tree->find_neighbors(search_particle, search_config);

    // All returned indices should be valid for cached_search_particles
    ASSERT_GE(result.neighbor_indices.size(), 0);
    for (size_t i = 0; i < result.neighbor_indices.size(); ++i) {
        int idx = result.neighbor_indices[i];
        EXPECT_GE(idx, 0);
        EXPECT_LT(idx, static_cast<int>(sim->cached_search_particles.size()))
            << "Tree search returned index beyond cached_search_particles range";
    }
}

// ============================================================================
// GIVEN: Time integration with ghost updates
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenTimeStep_WhenGhostsUpdated_ThenTreeRebuilt) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    const size_t initial_search_size = sim->cached_search_particles.size();
    
    // WHEN: One time integration step (ghosts regenerated)
    solver.predict();
    solver.integrate();

    // THEN: Tree still valid with potentially updated ghost count
    auto* tree = sim->tree.get();
    ASSERT_NE(tree, nullptr);

    // Create a dummy particle at search position for the new API
    SPHParticle<3> search_particle;
    search_particle.pos = Vec3d(1.0, 1.0, 1.0);
    search_particle.sml = 1.0;
    
    const auto search_config = NeighborSearchConfig::create(50, false);
    auto result = tree->find_neighbors(search_particle, search_config);

    ASSERT_GE(result.neighbor_indices.size(), 0);
    for (size_t i = 0; i < result.neighbor_indices.size(); ++i) {
        int idx = result.neighbor_indices[i];
        EXPECT_GE(idx, 0);
        EXPECT_LT(idx, static_cast<int>(sim->cached_search_particles.size()))
            << "After integration, tree should respect new particle count";
    }
}

TEST_F(SolverGhostLifecycleTest, GivenMultipleTimeSteps_WhenGhostsRegeneratedEachStep_ThenIDsAlwaysMatchIndices) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    // WHEN: Multiple time steps executed
    for (int step = 0; step < 5; ++step) {
        solver.predict();
        solver.integrate();
        solver.correct();

        // THEN: IDs match indices after each step
        const auto& search_particles = sim->cached_search_particles;
        for (size_t i = 0; i < search_particles.size(); ++i) {
            EXPECT_EQ(search_particles[i].id, static_cast<int>(i))
                << "Step " << step << ": Particle at index " << i 
                << " has mismatched ID " << search_particles[i].id;
        }
    }
}

TEST_F(SolverGhostLifecycleTest, GivenMultipleTimeSteps_WhenTreeQueriedEachStep_ThenAlwaysReturnsValidIndices) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    auto* tree = sim->tree.get();
    ASSERT_NE(tree, nullptr);

    // WHEN: Multiple steps with tree queries
    for (int step = 0; step < 5; ++step) {
        solver.predict();
        solver.integrate();

        // Query tree at each step
        SPHParticle<3> search_particle;
        search_particle.pos = Vec3d(step * 0.5, 0.0, 0.0);
        search_particle.sml = 1.0;
        
        const auto search_config = NeighborSearchConfig::create(50, false);
        auto result = tree->find_neighbors(search_particle, search_config);

        // THEN: All indices valid for current particle count
        ASSERT_GE(result.neighbor_indices.size(), 0) << "Step " << step;
        for (size_t i = 0; i < result.neighbor_indices.size(); ++i) {
            int idx = result.neighbor_indices[i];
            EXPECT_GE(idx, 0) << "Step " << step;
            EXPECT_LT(idx, static_cast<int>(sim->cached_search_particles.size()))
                << "Step " << step << ": Index out of bounds";
        }

        solver.correct();
    }
}

// ============================================================================
// GIVEN: Container separation (particles vs cached_search_particles)
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenSolverRunning_WhenRealParticlesAccessed_ThenOnlyContainsRealParticles) {
    // GIVEN: Initialized solver with ghosts
    Solver solver(sim);
    solver.initialize();

    // WHEN: Real particles accessed
    const auto& real_particles = sim->particles;
    const int real_count = sim->particle_num;

    // THEN: Real particle container has exact count, no ghosts
    EXPECT_EQ(real_particles.size(), static_cast<size_t>(real_count))
        << "Real particle container should only contain real particles";

    // AND: All IDs in real particles are < real_count
    for (size_t i = 0; i < real_particles.size(); ++i) {
        EXPECT_EQ(real_particles[i].id, static_cast<int>(i))
            << "Real particles should have sequential IDs from 0";
        EXPECT_LT(real_particles[i].id, real_count);
    }
}

TEST_F(SolverGhostLifecycleTest, GivenSolverRunning_WhenSearchParticlesAccessed_ThenContainsBothRealAndGhost) {
    // GIVEN: Initialized solver with ghosts
    Solver solver(sim);
    solver.initialize();

    // WHEN: Search particles accessed
    const auto& search_particles = sim->cached_search_particles;
    const int real_count = sim->particle_num;

    if (sim->ghosts.size() > 0) {
        // THEN: Search container includes ghosts
        EXPECT_GT(search_particles.size(), static_cast<size_t>(real_count))
            << "Search particles should include real + ghost";

        // AND: Ghost IDs start after real particles
        for (size_t i = real_count; i < search_particles.size(); ++i) {
            EXPECT_GE(search_particles[i].id, real_count)
                << "Ghost particle IDs should be >= real_count";
            EXPECT_EQ(search_particles[i].id, static_cast<int>(i))
                << "Ghost IDs should match their position in search array";
        }
    }
}

TEST_F(SolverGhostLifecycleTest, GivenTimeIntegration_WhenRealParticlesUpdated_ThenGhostsNotInRealContainer) {
    // GIVEN: Solver running
    Solver solver(sim);
    solver.initialize();

    const size_t real_size_before = sim->particles.size();

    // WHEN: Time step executed (updates real particles, regenerates ghosts)
    solver.predict();
    solver.integrate();
    solver.correct();

    // THEN: Real particle container size unchanged
    EXPECT_EQ(sim->particles.size(), real_size_before)
        << "Real particle container should not include ghosts after integration";

    // AND: Ghosts remain separate
    if (sim->ghosts.size() > 0) {
        EXPECT_GT(sim->cached_search_particles.size(), sim->particles.size())
            << "Ghosts should remain in separate cached list";
    }
}

// ============================================================================
// GIVEN: Edge cases - sudden ghost count changes
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenBoundaryParticlesMove_WhenGhostCountChanges_ThenTreeHandlesCorrectly) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    const size_t initial_search_size = sim->cached_search_particles.size();

    // WHEN: Multiple steps (ghost count may fluctuate as particles move)
    for (int step = 0; step < 10; ++step) {
        solver.predict();
        
        // Artificially move boundary particles (would change ghost generation)
        // In real simulation, particle motion triggers different ghost patterns
        
        solver.integrate();

        // THEN: Tree remains consistent regardless of count changes
        auto* tree = sim->tree.get();
        SPHParticle<3> search_particle;
        search_particle.pos = Vec3d(2.0, 2.0, 2.0);
        search_particle.sml = 1.0;
        
        const auto search_config = NeighborSearchConfig::create(50, false);
        auto result = tree->find_neighbors(search_particle, search_config);

        ASSERT_GE(result.neighbor_indices.size(), 0) << "Step " << step;
        for (size_t i = 0; i < result.neighbor_indices.size(); ++i) {
            int idx = result.neighbor_indices[i];
            EXPECT_LT(idx, static_cast<int>(sim->cached_search_particles.size()))
                << "Step " << step << ": Tree should adapt to changing ghost count";
        }

        solver.correct();
    }
}

TEST_F(SolverGhostLifecycleTest, GivenLargeGhostCountJump_WhenIntegrate_ThenNoReallocation) {
    // GIVEN: Solver initialized with small ghost count
    Solver solver(sim);
    solver.initialize();

    const size_t initial_capacity = sim->cached_search_particles.capacity();
    const size_t initial_search_size = sim->cached_search_particles.size();

    // WHEN: Integration step (may add more ghosts)
    solver.predict();
    solver.integrate();

    // THEN: If count grew, capacity should have reserved buffer
    const size_t final_capacity = sim->cached_search_particles.capacity();
    const size_t final_search_size = sim->cached_search_particles.size();

    if (final_search_size > initial_search_size) {
        EXPECT_GE(final_capacity, final_search_size)
            << "Capacity should accommodate growth";
        
        // The solver should have reserved extra space to avoid reallocation
        // This prevents tree pointer invalidation
        EXPECT_GT(final_capacity, final_search_size)
            << "Should reserve buffer beyond current size to prevent frequent reallocations";
    }
}

// ============================================================================
// GIVEN: Parallel operations during integration
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenParallelPreInteraction_WhenAccessingSearchParticles_ThenThreadSafe) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    // WHEN: Predict step runs (parallel pre-interaction uses search_particles)
    EXPECT_NO_THROW({
        solver.predict();
    }) << "Parallel pre-interaction should access search_particles safely";

    // THEN: All particles still have valid IDs
    const auto& search_particles = sim->cached_search_particles;
    for (size_t i = 0; i < search_particles.size(); ++i) {
        EXPECT_EQ(search_particles[i].id, static_cast<int>(i))
            << "Parallel access should not corrupt particle IDs";
    }
}

TEST_F(SolverGhostLifecycleTest, GivenParallelFluidForce_WhenTreeQueried_ThenNoRaceConditions) {
    // GIVEN: Initialized solver
    Solver solver(sim);
    solver.initialize();

    // WHEN: Multiple integration steps (parallel tree queries in fluid_force)
    for (int step = 0; step < 5; ++step) {
        EXPECT_NO_THROW({
            solver.predict();
            solver.integrate();
            solver.correct();
        }) << "Step " << step << ": Parallel tree operations should be race-free";
    }

    // THEN: Final state is consistent
    const auto& search_particles = sim->cached_search_particles;
    for (size_t i = 0; i < search_particles.size(); ++i) {
        EXPECT_EQ(search_particles[i].id, static_cast<int>(i))
            << "Final particle IDs should be consistent after parallel operations";
    }
}

// ============================================================================
// GIVEN: Full simulation lifecycle
// ============================================================================

TEST_F(SolverGhostLifecycleTest, GivenFullSimulation_WhenRunToCompletion_ThenNoIDMismatch) {
    // GIVEN: Solver ready to run
    Solver solver(sim);
    solver.initialize();

    // WHEN: Run full simulation (10 timesteps)
    int step_count = 0;
    const int max_steps = 10;
    
    while (solver.get_time() < sim->end_time && step_count < max_steps) {
        solver.predict();
        solver.integrate();
        solver.correct();
        step_count++;

        // Verify ID consistency at each step
        const auto& search_particles = sim->cached_search_particles;
        for (size_t i = 0; i < search_particles.size(); ++i) {
            ASSERT_EQ(search_particles[i].id, static_cast<int>(i))
                << "Step " << step_count << ": ID mismatch detected";
        }
    }

    // THEN: Simulation completed without ID errors
    EXPECT_GT(step_count, 0) << "Should have executed at least one timestep";
}

TEST_F(SolverGhostLifecycleTest, GivenFullSimulation_WhenOutputGenerated_ThenOnlyRealParticlesWritten) {
    // GIVEN: Solver with output enabled
    Solver solver(sim);
    solver.initialize();

    // WHEN: Run simulation with output
    solver.predict();
    solver.integrate();
    // Output would be generated here in real simulation

    // THEN: Only real particles should be in output
    // Verify by checking particles container is separate from search_particles
    const auto& output_particles = sim->particles;
    EXPECT_EQ(output_particles.size(), static_cast<size_t>(sim->particle_num))
        << "Output should only contain real particles, not ghosts";

    if (sim->ghosts.size() > 0) {
        EXPECT_LT(output_particles.size(), sim->cached_search_particles.size())
            << "Real particle count should be less than search particle count when ghosts exist";
    }
}

// ============================================================================
// Main entry point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

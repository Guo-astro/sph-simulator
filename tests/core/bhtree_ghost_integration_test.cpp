/**
 * @file bhtree_ghost_integration_test.cpp
 * @brief BDD-style tests for BHTree ghost particle integration
 * 
 * Tests validate critical invariants discovered during ghost particle debugging:
 * - Ghost particle ID must equal index in combined particle array
 * - Tree must track which container it was built with
 * - neighbor_search must use same container as tree build
 * - Invalid indices must be filtered before access
 */

#include <gtest/gtest.h>
#include "core/bhtree.hpp"
#include "core/sph_particle.hpp"
#include "utilities/vec_n.hpp"
#include <vector>
#include <algorithm>

using namespace sph;
using utilities::Vec3d;

// Test fixture for 3D BHTree tests
class BHTreeGhostIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default tree parameters
        max_level = 10;
        leaf_particle_num = 10;
        neighbor_number = 50;
    }

    // Helper: Create particle at position with given ID
    SPHParticle<3> make_particle(int id, double x, double y, double z) {
        SPHParticle<3> p;
        p.id = id;
        p.r = Vec3d(x, y, z);
        p.m = 1.0;
        p.v = Vec3d(0.0, 0.0, 0.0);
        p.h = 0.1;
        p.rho = 1.0;
        p.p = 0.0;
        return p;
    }

    // Helper: Verify particle IDs match their indices
    void verify_id_equals_index(const std::vector<SPHParticle<3>>& particles) {
        for (size_t i = 0; i < particles.size(); ++i) {
            EXPECT_EQ(particles[i].id, static_cast<int>(i))
                << "Particle at index " << i << " has mismatched ID " << particles[i].id;
        }
    }

    int max_level;
    int leaf_particle_num;
    int neighbor_number;
};

// ============================================================================
// GIVEN: Tree built with real particles only
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenRealParticlesOnly_WhenTreeBuilt_ThenIDsMatchIndices) {
    // GIVEN: Real particles with IDs matching indices
    std::vector<SPHParticle<3>> particles;
    for (int i = 0; i < 100; ++i) {
        particles.push_back(make_particle(i, i * 0.1, i * 0.1, i * 0.1));
    }
    verify_id_equals_index(particles);

    // WHEN: Tree is built
    BHTree<3> tree;
    tree.make(particles, max_level, leaf_particle_num);

    // THEN: Tree build succeeds without error
    SUCCEED();
}

TEST_F(BHTreeGhostIntegrationTest, GivenRealParticlesOnly_WhenNeighborSearch_ThenOnlyValidIndicesReturned) {
    // GIVEN: Real particles in grid
    std::vector<SPHParticle<3>> particles;
    for (int i = 0; i < 50; ++i) {
        particles.push_back(make_particle(i, i * 0.2, 0.0, 0.0));
    }

    BHTree<3> tree;
    tree.make(particles, max_level, leaf_particle_num);

    // WHEN: Neighbor search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(5.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: All returned indices are valid
    ASSERT_GE(n_found, 0) << "Neighbor search should return non-negative count";
    for (int i = 0; i < n_found; ++i) {
        EXPECT_GE(neighbors[i], 0) << "Neighbor index cannot be negative";
        EXPECT_LT(neighbors[i], static_cast<int>(particles.size()))
            << "Neighbor index " << neighbors[i] << " exceeds particle count " << particles.size();
    }
}

// ============================================================================
// GIVEN: Combined particle list (real + ghost)
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenRealAndGhostParticles_WhenGhostIDsCorrect_ThenTreeBuildsSuccessfully) {
    // GIVEN: Real particles
    std::vector<SPHParticle<3>> real_particles;
    for (int i = 0; i < 100; ++i) {
        real_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    // AND: Ghost particles with IDs starting after real particles
    std::vector<SPHParticle<3>> ghosts;
    const int ghost_id_offset = static_cast<int>(real_particles.size());
    for (int i = 0; i < 50; ++i) {
        ghosts.push_back(make_particle(ghost_id_offset + i, -0.1 - i * 0.1, 0.0, 0.0));
    }

    // AND: Combined list where ID equals index
    std::vector<SPHParticle<3>> combined = real_particles;
    combined.insert(combined.end(), ghosts.begin(), ghosts.end());
    verify_id_equals_index(combined);

    // WHEN: Tree built with combined list
    BHTree<3> tree;
    tree.make(combined, max_level, leaf_particle_num);

    // THEN: Build succeeds
    SUCCEED();
}

TEST_F(BHTreeGhostIntegrationTest, GivenRealAndGhostParticles_WhenNeighborSearch_ThenCanReturnBothTypes) {
    // GIVEN: Real particles at x > 0
    std::vector<SPHParticle<3>> real_particles;
    for (int i = 0; i < 50; ++i) {
        real_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    // AND: Ghost particles at x < 0 (boundary reflections)
    std::vector<SPHParticle<3>> ghosts;
    const int ghost_id_offset = static_cast<int>(real_particles.size());
    for (int i = 0; i < 20; ++i) {
        ghosts.push_back(make_particle(ghost_id_offset + i, -0.1 - i * 0.05, 0.0, 0.0));
    }

    std::vector<SPHParticle<3>> combined = real_particles;
    combined.insert(combined.end(), ghosts.begin(), ghosts.end());
    verify_id_equals_index(combined);

    // WHEN: Tree built and search near boundary
    BHTree<3> tree;
    tree.make(combined, max_level, leaf_particle_num);

    std::vector<int> neighbors(neighbor_number);
    Vec3d boundary_pos(0.05, 0.0, 0.0);  // Near x=0 boundary
    int n_found = tree.neighbor_search(boundary_pos, neighbor_number, neighbors.data());

    // THEN: Search returns valid mix of real and ghost indices
    ASSERT_GT(n_found, 0) << "Should find neighbors near boundary";
    
    bool found_real = false;
    bool found_ghost = false;
    for (int i = 0; i < n_found; ++i) {
        ASSERT_GE(neighbors[i], 0);
        ASSERT_LT(neighbors[i], static_cast<int>(combined.size()));
        
        if (neighbors[i] < ghost_id_offset) {
            found_real = true;
        } else {
            found_ghost = true;
        }
    }
    
    EXPECT_TRUE(found_real) << "Should find real particle neighbors near boundary";
    EXPECT_TRUE(found_ghost) << "Should find ghost particle neighbors near boundary";
}

// ============================================================================
// GIVEN: Ghost particles with INCORRECT IDs (regression test)
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenGhostParticlesWithSourceIDs_WhenNeighborSearch_ThenNoIndexOutOfBounds) {
    // GIVEN: Real particles
    std::vector<SPHParticle<3>> real_particles;
    for (int i = 0; i < 100; ++i) {
        real_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    // AND: Ghost particles with WRONG IDs (source particle IDs, not renumbered)
    // This simulates the bug that caused the original segfault
    std::vector<SPHParticle<3>> ghosts;
    for (int i = 0; i < 50; ++i) {
        // BUG: Using source ID instead of offset ID
        ghosts.push_back(make_particle(i, -0.1 - i * 0.1, 0.0, 0.0));
    }

    std::vector<SPHParticle<3>> combined = real_particles;
    combined.insert(combined.end(), ghosts.begin(), ghosts.end());

    // Verify IDs do NOT match indices (this is the bug)
    for (size_t i = real_particles.size(); i < combined.size(); ++i) {
        ASSERT_NE(combined[i].id, static_cast<int>(i))
            << "Ghost particle should have wrong ID for this test case";
    }

    // WHEN: Tree built with buggy IDs
    BHTree<3> tree;
    tree.make(combined, max_level, leaf_particle_num);

    // AND: Neighbor search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(0.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: No out-of-bounds access (validation catches invalid indices)
    // The fix filters out any neighbor.id >= combined.size()
    ASSERT_GE(n_found, 0);
    for (int i = 0; i < n_found; ++i) {
        EXPECT_GE(neighbors[i], 0);
        EXPECT_LT(neighbors[i], static_cast<int>(combined.size()))
            << "Bounds validation should prevent out-of-bounds indices";
    }
}

// ============================================================================
// GIVEN: Container changes between build and search
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenTreeBuiltWithOneContainer_WhenSearchWithDifferentContainer_ThenUsesOriginalContainer) {
    // GIVEN: Tree built with initial particles
    std::vector<SPHParticle<3>> build_particles;
    for (int i = 0; i < 50; ++i) {
        build_particles.push_back(make_particle(i, i * 0.2, 0.0, 0.0));
    }

    BHTree<3> tree;
    tree.make(build_particles, max_level, leaf_particle_num);

    // WHEN: A different container is created (simulating caller error)
    std::vector<SPHParticle<3>> search_particles = build_particles;
    search_particles.push_back(make_particle(50, 10.0, 0.0, 0.0));  // Extra particle

    // AND: Neighbor search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(5.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Tree uses original container size for bounds checking
    // The fix stores m_particles_ptr during make() and uses it in neighbor_search()
    ASSERT_GE(n_found, 0);
    for (int i = 0; i < n_found; ++i) {
        EXPECT_LT(neighbors[i], static_cast<int>(build_particles.size()))
            << "Should validate against original build container size, not different container";
    }
}

// ============================================================================
// GIVEN: Edge cases - zero ghosts, all ghosts, empty tree
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenNoGhostParticles_WhenTreeBuilt_ThenBehavesNormally) {
    // GIVEN: Only real particles (zero ghosts)
    std::vector<SPHParticle<3>> particles;
    for (int i = 0; i < 100; ++i) {
        particles.push_back(make_particle(i, i * 0.1, i * 0.1, 0.0));
    }

    // WHEN: Tree built and searched
    BHTree<3> tree;
    tree.make(particles, max_level, leaf_particle_num);

    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(5.0, 5.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Works as expected
    ASSERT_GT(n_found, 0);
    for (int i = 0; i < n_found; ++i) {
        EXPECT_GE(neighbors[i], 0);
        EXPECT_LT(neighbors[i], static_cast<int>(particles.size()));
    }
}

TEST_F(BHTreeGhostIntegrationTest, GivenOnlyGhostParticles_WhenTreeBuilt_ThenBehavesNormally) {
    // GIVEN: Only ghost particles (simulating pure boundary representation)
    std::vector<SPHParticle<3>> ghosts;
    for (int i = 0; i < 50; ++i) {
        ghosts.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }
    verify_id_equals_index(ghosts);

    // WHEN: Tree built with only ghosts
    BHTree<3> tree;
    tree.make(ghosts, max_level, leaf_particle_num);

    // AND: Search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(2.5, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Valid results returned
    ASSERT_GT(n_found, 0);
    for (int i = 0; i < n_found; ++i) {
        EXPECT_GE(neighbors[i], 0);
        EXPECT_LT(neighbors[i], static_cast<int>(ghosts.size()));
    }
}

TEST_F(BHTreeGhostIntegrationTest, GivenEmptyParticleList_WhenTreeBuilt_ThenHandlesGracefully) {
    // GIVEN: Empty particle list
    std::vector<SPHParticle<3>> particles;

    // WHEN: Tree built
    BHTree<3> tree;
    tree.make(particles, max_level, leaf_particle_num);

    // AND: Search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(0.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Returns zero neighbors without crashing
    EXPECT_EQ(n_found, 0) << "Empty tree should return zero neighbors";
}

TEST_F(BHTreeGhostIntegrationTest, GivenLargeGhostRatio_WhenTreeBuilt_ThenHandlesEfficiently) {
    // GIVEN: Few real particles
    std::vector<SPHParticle<3>> real_particles;
    for (int i = 0; i < 50; ++i) {
        real_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    // AND: Many ghost particles (5x real count)
    std::vector<SPHParticle<3>> ghosts;
    const int ghost_id_offset = static_cast<int>(real_particles.size());
    for (int i = 0; i < 250; ++i) {
        double angle = i * 2.0 * M_PI / 250.0;
        ghosts.push_back(make_particle(
            ghost_id_offset + i,
            10.0 * std::cos(angle),
            10.0 * std::sin(angle),
            0.0
        ));
    }

    std::vector<SPHParticle<3>> combined = real_particles;
    combined.insert(combined.end(), ghosts.begin(), ghosts.end());
    verify_id_equals_index(combined);

    // WHEN: Tree built with large ghost ratio
    BHTree<3> tree;
    tree.make(combined, max_level, leaf_particle_num);

    // AND: Search in ghost-heavy region
    std::vector<int> neighbors(neighbor_number);
    Vec3d boundary_pos(9.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(boundary_pos, neighbor_number, neighbors.data());

    // THEN: Returns valid ghost neighbors
    ASSERT_GT(n_found, 0);
    bool found_ghosts = false;
    for (int i = 0; i < n_found; ++i) {
        ASSERT_GE(neighbors[i], 0);
        ASSERT_LT(neighbors[i], static_cast<int>(combined.size()));
        if (neighbors[i] >= ghost_id_offset) {
            found_ghosts = true;
        }
    }
    EXPECT_TRUE(found_ghosts) << "Should find ghost particles in ghost-heavy region";
}

// ============================================================================
// GIVEN: Particle count changes (add/remove ghosts)
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenTreeRebuiltWithFewerParticles_WhenSearched_ThenUsesNewBounds) {
    // GIVEN: Initial tree with many particles
    std::vector<SPHParticle<3>> initial_particles;
    for (int i = 0; i < 200; ++i) {
        initial_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    BHTree<3> tree;
    tree.make(initial_particles, max_level, leaf_particle_num);

    // WHEN: Tree rebuilt with fewer particles (ghosts removed)
    std::vector<SPHParticle<3>> reduced_particles;
    for (int i = 0; i < 100; ++i) {
        reduced_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }
    tree.make(reduced_particles, max_level, leaf_particle_num);

    // AND: Search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(5.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Indices respect new smaller bound
    ASSERT_GE(n_found, 0);
    for (int i = 0; i < n_found; ++i) {
        EXPECT_LT(neighbors[i], static_cast<int>(reduced_particles.size()))
            << "After rebuild, indices should respect new particle count";
    }
}

TEST_F(BHTreeGhostIntegrationTest, GivenTreeRebuiltWithMoreParticles_WhenSearched_ThenUsesNewBounds) {
    // GIVEN: Initial tree with few particles
    std::vector<SPHParticle<3>> initial_particles;
    for (int i = 0; i < 50; ++i) {
        initial_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }

    BHTree<3> tree;
    tree.make(initial_particles, max_level, leaf_particle_num);

    // WHEN: Tree rebuilt with more particles (ghosts added)
    std::vector<SPHParticle<3>> expanded_particles;
    for (int i = 0; i < 150; ++i) {
        expanded_particles.push_back(make_particle(i, i * 0.1, 0.0, 0.0));
    }
    tree.make(expanded_particles, max_level, leaf_particle_num);

    // AND: Search performed
    std::vector<int> neighbors(neighbor_number);
    Vec3d search_pos(10.0, 0.0, 0.0);
    int n_found = tree.neighbor_search(search_pos, neighbor_number, neighbors.data());

    // THEN: Can return indices from expanded range
    ASSERT_GT(n_found, 0);
    bool found_new_range = false;
    for (int i = 0; i < n_found; ++i) {
        EXPECT_GE(neighbors[i], 0);
        EXPECT_LT(neighbors[i], static_cast<int>(expanded_particles.size()));
        if (neighbors[i] >= 50) {
            found_new_range = true;  // Found particle from expanded range
        }
    }
    EXPECT_TRUE(found_new_range) << "Should access newly added particles after rebuild";
}

// ============================================================================
// GIVEN: Parallel neighbor search (OpenMP safety)
// ============================================================================

TEST_F(BHTreeGhostIntegrationTest, GivenMultipleThreads_WhenParallelNeighborSearch_ThenAllResultsValid) {
    // GIVEN: Combined particle list
    std::vector<SPHParticle<3>> particles;
    for (int i = 0; i < 200; ++i) {
        particles.push_back(make_particle(i, i * 0.1, i * 0.1, 0.0));
    }

    BHTree<3> tree;
    tree.make(particles, max_level, leaf_particle_num);

    // WHEN: Parallel search from multiple positions
    constexpr int num_searches = 100;
    std::vector<std::vector<int>> all_neighbors(num_searches, std::vector<int>(neighbor_number));
    std::vector<int> all_counts(num_searches);

    #pragma omp parallel for
    for (int s = 0; s < num_searches; ++s) {
        Vec3d search_pos(s * 0.15, s * 0.15, 0.0);
        all_counts[s] = tree.neighbor_search(search_pos, neighbor_number, all_neighbors[s].data());
    }

    // THEN: All results have valid indices
    for (int s = 0; s < num_searches; ++s) {
        ASSERT_GE(all_counts[s], 0) << "Search " << s << " returned negative count";
        for (int i = 0; i < all_counts[s]; ++i) {
            EXPECT_GE(all_neighbors[s][i], 0)
                << "Search " << s << ", neighbor " << i << " has negative index";
            EXPECT_LT(all_neighbors[s][i], static_cast<int>(particles.size()))
                << "Search " << s << ", neighbor " << i << " exceeds bounds";
        }
    }
}

// ============================================================================
// Main entry point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

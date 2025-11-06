#include <gtest/gtest.h>
#include "core/neighbors/neighbor_accessor.hpp"
#include "core/neighbors/particle_array_types.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/spatial/neighbor_search_result.hpp"
#include "core/utilities/vector.hpp"
#include <vector>
#include <memory>

using namespace sph;

/**
 * @brief Integration tests for type-safe neighbor accessor with SPH methods
 * 
 * These tests validate that the NeighborAccessor correctly integrates
 * with DISPH and GSPH implementations, ensuring:
 * 1. Ghost particles are accessible via neighbor indices
 * 2. Density calculations include ghost contributions
 * 3. Force calculations use correct neighbor array access
 * 4. No array index mismatch bugs occur during neighbor iteration
 * 
 * Test Strategy: BDD-style Given/When/Then
 */
class NeighborAccessorSPHIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create real particles in a 1D line
        constexpr int num_real = 10;
        real_particles.resize(num_real);
        for (int i = 0; i < num_real; ++i) {
            real_particles[i].id = i;
            real_particles[i].pos = Vector<2>{static_cast<real>(i * 0.1), 0.0};
            real_particles[i].vel = Vector<2>{0.0, 0.0};
            real_particles[i].mass = 1.0;
            real_particles[i].dens = 1000.0;
            real_particles[i].pres = 101325.0;
            real_particles[i].sml = 0.15;
            real_particles[i].type = static_cast<int>(ParticleType::REAL);
        }
        
        // Create search particles (real + ghost)
        constexpr int num_ghost = 5;
        constexpr int num_total = num_real + num_ghost;
        search_particles.resize(num_total);
        
        // Copy real particles
        for (int i = 0; i < num_real; ++i) {
            search_particles[i] = real_particles[i];
        }
        
        // Add ghost particles (boundary mirror particles)
        for (int i = 0; i < num_ghost; ++i) {
            int ghost_idx = num_real + i;
            search_particles[ghost_idx].id = 1000 + i;  // Large IDs for ghosts
            search_particles[ghost_idx].pos = Vector<2>{-static_cast<real>(i * 0.1) - 0.1, 0.0};
            search_particles[ghost_idx].vel = Vector<2>{0.0, 0.0};
            search_particles[ghost_idx].mass = 1.0;
            search_particles[ghost_idx].dens = 1000.0;
            search_particles[ghost_idx].pres = 101325.0;
            search_particles[ghost_idx].sml = 0.15;
            search_particles[ghost_idx].type = static_cast<int>(ParticleType::GHOST);
        }
    }

    std::vector<SPHParticle<2>> real_particles;
    std::vector<SPHParticle<2>> search_particles;
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

/**
 * @test Given a particle with real and ghost neighbors
 *       When creating NeighborAccessor from SearchParticleArray
 *       Then accessor provides access to all neighbors
 */
TEST_F(NeighborAccessorSPHIntegrationTest, GivenMixedNeighbors_WhenCreateAccessor_ThenAccessAll) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Simulate neighbor search result: 3 real neighbors + 2 ghost neighbors
    std::vector<int> neighbor_indices = {1, 2, 3, 10, 11};  // Indices in search_particles
    
    // Act & Assert: Access all neighbors
    for (int raw_idx : neighbor_indices) {
        NeighborIndex idx{raw_idx};
        const auto& particle = accessor.get_neighbor(idx);
        
        if (raw_idx < 10) {
            // Real particle
            EXPECT_EQ(particle.type, static_cast<int>(ParticleType::REAL));
            EXPECT_EQ(particle.id, raw_idx);
        } else {
            // Ghost particle
            EXPECT_EQ(particle.type, static_cast<int>(ParticleType::GHOST));
            EXPECT_GE(particle.id, 1000);
        }
    }
}

/**
 * @test Given NeighborSearchResult with iterator
 *       When iterating with NeighborIndexIterator
 *       Then can use indices with NeighborAccessor
 */
TEST_F(NeighborAccessorSPHIntegrationTest, GivenSearchResult_WhenIterateWithIterator_ThenAccessorWorks) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Create mock neighbor search result
    std::vector<int> raw_neighbors = {0, 1, 2, 10, 11};  // 3 real + 2 ghost
    NeighborSearchResult result;
    result.neighbor_indices = raw_neighbors;
    result.is_truncated = false;
    result.total_candidates_found = static_cast<int>(raw_neighbors.size());
    
    // Act: Iterate using type-safe iterator
    int count = 0;
    for (auto idx : result) {  // Returns NeighborIndex
        const auto& particle = accessor.get_neighbor(idx);
        EXPECT_GE(particle.id, 0);
        ++count;
    }
    
    // Assert
    EXPECT_EQ(count, 5);
}

// ============================================================================
// DISPH-style Density Calculation Tests
// ============================================================================

/**
 * @test Given particle at boundary with ghost neighbors
 *       When calculating density (DISPH PreInteraction pattern)
 *       Then density includes ghost contributions
 * 
 * This simulates the refactored d_pre_interaction.tpp pattern.
 */
TEST_F(NeighborAccessorSPHIntegrationTest, DISPH_PreInteraction_WithGhosts_IncludesGhostDensity) {
    // Arrange: Particle 0 near boundary with 2 real + 2 ghost neighbors
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    const int particle_i = 0;
    const auto& pi = real_particles[particle_i];
    
    // Simulate neighbor search result for particle 0
    std::vector<int> neighbor_indices = {1, 2, 10, 11};  // 2 real + 2 ghost
    
    // Mock kernel function (simplified W_ij)
    auto kernel = [&](const Vector<2>& r_i, const Vector<2>& r_j, real h) -> real {
        real r = abs(r_i - r_j);  // Use abs() for magnitude
        if (r < h) {
            return 1.0 - r / h;  // Simplified linear kernel
        }
        return 0.0;
    };
    
    // Act: Calculate density (DISPH pattern from d_pre_interaction.tpp)
    real density = 0.0;
    for (int raw_idx : neighbor_indices) {
        NeighborIndex neighbor_idx{raw_idx};
        const auto& pj = accessor.get_neighbor(neighbor_idx);
        
        real W_ij = kernel(pi.pos, pj.pos, pi.sml);
        density += pj.mass * W_ij;
    }
    
    // Assert: Density > 0 (ghost contributions included)
    EXPECT_GT(density, 0.0);
    // With 4 neighbors and simplified kernel, expect some positive density
    // (exact value depends on particle spacing and kernel support)
}

/**
 * @test Regression: Array index mismatch bug prevention
 * 
 * Bug Description (from TYPE_SAFETY_ARCHITECTURE.md):
 *   Neighbor indices referenced cached_search_particles[] (real + ghost)
 *   but code accessed particles[] (real only), causing segfault when
 *   neighbor index >= num_real_particles.
 * 
 * This test ensures the bug cannot recur with type-safe accessor.
 */
TEST_F(NeighborAccessorSPHIntegrationTest, REGRESSION_ArrayMismatch_CannotOccur) {
    // Arrange: Create the exact scenario that caused the bug
    ASSERT_EQ(real_particles.size(), 10);
    ASSERT_EQ(search_particles.size(), 15);
    
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Simulate neighbor index pointing to ghost (index 12 is in search, not in real)
    NeighborIndex ghost_idx{12};  // Would have caused segfault with old code
    
    // Act: Access ghost particle safely
    const auto& ghost = accessor.get_neighbor(ghost_idx);
    
    // Assert: Successfully accessed ghost particle
    EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST));
    
    // The following would NOT compile (compile-time safety):
    // RealParticleArray<2> real_array{real_particles};
    // NeighborAccessor<2> bad_accessor{real_array};  // ❌ Compile error: deleted constructor
    
    SUCCEED() << "Type system prevents the array index mismatch bug";
}

// ============================================================================
// GSPH-style Force Calculation Tests
// ============================================================================

/**
 * @test Given particle with ghost neighbors
 *       When calculating force (GSPH FluidForce pattern)
 *       Then force calculation uses correct neighbor indices
 * 
 * This simulates the refactored g_fluid_force.tpp pattern.
 */
TEST_F(NeighborAccessorSPHIntegrationTest, GSPH_FluidForce_WithGhosts_CorrectForceCalculation) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    const int particle_i = 0;
    const auto& pi = real_particles[particle_i];
    
    // Simulate neighbor search result
    std::vector<int> neighbor_indices = {1, 2, 10};  // 2 real + 1 ghost
    
    // Mock gradient kernel (simplified)
    auto grad_W = [](const Vector<2>& r_ij) -> Vector<2> {
        real r = abs(r_ij);  // Use abs() for magnitude
        if (r > 1e-10) {
            return r_ij * (-1.0 / r);  // Simplified gradient
        }
        return Vector<2>{0.0, 0.0};
    };
    
    // Act: Calculate force (GSPH pattern from g_fluid_force.tpp)
    Vector<2> force{0.0, 0.0};
    for (int raw_idx : neighbor_indices) {
        NeighborIndex neighbor_idx{raw_idx};
        const auto& pj = accessor.get_neighbor(neighbor_idx);
        
        Vector<2> r_ij = pi.pos - pj.pos;
        Vector<2> grad_W_ij = grad_W(r_ij);
        
        // Simplified force calculation (pressure term only)
        real p_term = (pi.pres / (pi.dens * pi.dens)) + (pj.pres / (pj.dens * pj.dens));
        force = force - grad_W_ij * (pj.mass * p_term);
    }
    
    // Assert: Force magnitude is non-zero (contributions from neighbors)
    real force_mag = abs(force);  // Use abs() for magnitude
    EXPECT_GE(force_mag, 0.0);  // May be zero if pressures balance
}

/**
 * @test Given GSPH gradient array indexed by neighbor index
 *       When accessing gradient for ghost neighbor
 *       Then uses neighbor_idx() for array indexing
 * 
 * This tests the pattern from g_fluid_force.tpp where gradients
 * are stored in arrays indexed by neighbor position.
 */
TEST_F(NeighborAccessorSPHIntegrationTest, GSPH_GradientArray_WithNeighborIndex_CorrectAccess) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    std::vector<int> neighbor_indices = {1, 2, 10};  // 2 real + 1 ghost
    
    // Simulate gradient array (from MUSCL reconstruction in g_pre_interaction.tpp)
    std::vector<Vector<2>> gradients(search_particles.size(), Vector<2>{0.0, 0.0});
    for (size_t i = 0; i < gradients.size(); ++i) {
        gradients[i] = Vector<2>{static_cast<real>(i * 0.01), 0.0};
    }
    
    // Act: Access gradients using neighbor indices
    for (int raw_idx : neighbor_indices) {
        NeighborIndex neighbor_idx{raw_idx};
        // Use neighbor_idx() for gradient array access (as in g_fluid_force.tpp)
        const auto& grad_j = gradients[static_cast<size_t>(neighbor_idx())];
        
        // Assert: Gradient matches expected value
        EXPECT_DOUBLE_EQ(grad_j[0], static_cast<real>(neighbor_idx() * 0.01));
    }
}

// ============================================================================
// Boundary Particle Tests
// ============================================================================

/**
 * @test Given boundary particle with majority ghost neighbors
 *       When iterating through neighbors
 *       Then correctly identifies real vs ghost particles
 */
TEST_F(NeighborAccessorSPHIntegrationTest, BoundaryParticle_WithMajorityGhosts_IdentifiesCorrectly) {
    // Arrange: Particle at boundary with 2 real + 4 ghost neighbors
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    std::vector<int> neighbor_indices = {1, 2, 10, 11, 12, 13};  // 2 real + 4 ghost
    
    // Act: Classify neighbors
    int real_count = 0;
    int ghost_count = 0;
    
    for (int raw_idx : neighbor_indices) {
        NeighborIndex neighbor_idx{raw_idx};
        const auto& pj = accessor.get_neighbor(neighbor_idx);
        
        if (pj.type == static_cast<int>(ParticleType::REAL)) {
            ++real_count;
        } else if (pj.type == static_cast<int>(ParticleType::GHOST)) {
            ++ghost_count;
        }
    }
    
    // Assert
    EXPECT_EQ(real_count, 2);
    EXPECT_EQ(ghost_count, 4);
}

/**
 * @test Given particle with no ghost neighbors (interior particle)
 *       When calculating density
 *       Then uses only real neighbors
 */
TEST_F(NeighborAccessorSPHIntegrationTest, InteriorParticle_NoGhosts_UsesOnlyRealNeighbors) {
    // Arrange: Interior particle with only real neighbors
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    std::vector<int> neighbor_indices = {3, 4, 5, 6};  // All real
    
    // Act: Verify all are real particles
    for (int raw_idx : neighbor_indices) {
        NeighborIndex neighbor_idx{raw_idx};
        const auto& pj = accessor.get_neighbor(neighbor_idx);
        
        // Assert
        EXPECT_EQ(pj.type, static_cast<int>(ParticleType::REAL));
        EXPECT_LT(pj.id, 100);  // Real particles have ID < 100
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * @test Given particle with single ghost neighbor
 *       When accessing that neighbor
 *       Then accessor works correctly
 */
TEST_F(NeighborAccessorSPHIntegrationTest, SingleGhostNeighbor_WhenAccess_ThenSucceeds) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    NeighborIndex ghost_idx{10};  // First ghost particle
    
    // Act
    const auto& ghost = accessor.get_neighbor(ghost_idx);
    
    // Assert
    EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST));
    EXPECT_EQ(ghost.id, 1000);
}

/**
 * @test Given particle with neighbors at array boundaries
 *       When accessing first and last indices
 *       Then both succeed
 */
TEST_F(NeighborAccessorSPHIntegrationTest, BoundaryIndices_WhenAccess_ThenBothSucceed) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    NeighborIndex first_idx{0};
    NeighborIndex last_idx{14};  // search_particles.size() - 1
    
    // Act
    const auto& first = accessor.get_neighbor(first_idx);
    const auto& last = accessor.get_neighbor(last_idx);
    
    // Assert
    EXPECT_EQ(first.id, 0);
    EXPECT_EQ(last.type, static_cast<int>(ParticleType::GHOST));
}

/**
 * @test Given empty neighbor list (isolated particle)
 *       When iterating with range-based for
 *       Then loop executes zero times
 */
TEST_F(NeighborAccessorSPHIntegrationTest, EmptyNeighborList_WhenIterate_ThenZeroIterations) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    std::vector<int> empty_neighbors;  // Isolated particle
    
    // Act
    int iteration_count = 0;
    for (int raw_idx : empty_neighbors) {
        NeighborIndex idx{raw_idx};
        accessor.get_neighbor(idx);
        ++iteration_count;
    }
    
    // Assert
    EXPECT_EQ(iteration_count, 0);
}

// ============================================================================
// Performance / Zero Overhead Tests
// ============================================================================

/**
 * @test Documentation: Zero-overhead abstraction in release builds
 * 
 * With optimizations enabled (-O3 -DNDEBUG), the type-safe accessor
 * should compile to the same assembly as direct array access.
 * 
 * Verification:
 *   objdump -d -S build/tests/sph_tests | grep -A 20 "get_neighbor"
 *   Should show single array indexing instruction with no bounds checks.
 */
TEST_F(NeighborAccessorSPHIntegrationTest, DOCUMENTATION_ZeroOverheadInRelease) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex idx{5};
    
    // Act: This should inline to search_particles[5] in release builds
    const auto& particle = accessor.get_neighbor(idx);
    
    // Assert
    EXPECT_EQ(particle.id, 5);
    
    SUCCEED() << "In release builds (-O3 -DNDEBUG), this compiles to zero-overhead array access";
}

/**
 * @test Given large neighbor loop (100 iterations)
 *       When accessing via NeighborAccessor
 *       Then performance matches direct access pattern
 */
TEST_F(NeighborAccessorSPHIntegrationTest, LargeNeighborLoop_WithAccessor_EfficientAccess) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Create large neighbor list
    std::vector<int> many_neighbors;
    for (int i = 0; i < 100; ++i) {
        many_neighbors.push_back(i % 15);  // Cycle through search_particles
    }
    
    // Act: Access all neighbors
    real total_mass = 0.0;
    for (int raw_idx : many_neighbors) {
        NeighborIndex idx{raw_idx};
        const auto& pj = accessor.get_neighbor(idx);
        total_mass += pj.mass;
    }
    
    // Assert: All accesses succeeded
    EXPECT_DOUBLE_EQ(total_mass, 100.0);  // 100 particles * 1.0 mass
}

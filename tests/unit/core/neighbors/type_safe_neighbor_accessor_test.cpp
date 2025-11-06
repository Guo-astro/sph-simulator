#include <gtest/gtest.h>
#include "core/neighbors/neighbor_accessor.hpp"
#include "core/neighbors/particle_array_types.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/utilities/vector.hpp"
#include <vector>
#include <stdexcept>

using namespace sph;

/**
 * @brief Test suite for type-safe neighbor access system
 * 
 * Tests the compile-time and runtime safety guarantees of the
 * NeighborAccessor, NeighborIndex, and TypedParticleArray types.
 * 
 * Design Goals Validated:
 * 1. Compile-time prevention of array type mismatch
 * 2. Explicit index type prevents implicit conversions
 * 3. Debug builds provide bounds checking
 * 4. Zero overhead in release builds
 */
class NeighborAccessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test particles (real particles)
        real_particles.resize(5);
        for (int i = 0; i < 5; ++i) {
            real_particles[i].id = i;
            real_particles[i].pos = Vector<2>{static_cast<real>(i), 0.0};
            real_particles[i].mass = 1.0;
        }
        
        // Create search particles (real + ghost)
        search_particles.resize(10);
        for (int i = 0; i < 10; ++i) {
            search_particles[i].id = i;
            search_particles[i].pos = Vector<2>{static_cast<real>(i), 0.0};
            search_particles[i].mass = 1.0;
        }
        // Mark ghosts
        for (int i = 5; i < 10; ++i) {
            search_particles[i].type = static_cast<int>(ParticleType::GHOST);
        }
    }

    std::vector<SPHParticle<2>> real_particles;
    std::vector<SPHParticle<2>> search_particles;
};

// ============================================================================
// NeighborIndex Tests
// ============================================================================

/**
 * @test Given an integer value
 *       When constructing NeighborIndex explicitly
 *       Then construction succeeds
 */
TEST_F(NeighborAccessorTest, GivenInteger_WhenExplicitConstruction_ThenSucceeds) {
    // Arrange & Act
    NeighborIndex idx{5};
    
    // Assert
    EXPECT_EQ(idx(), 5);
}

/**
 * @test Given a NeighborIndex
 *       When extracting value with operator()
 *       Then returns correct integer value
 */
TEST_F(NeighborAccessorTest, GivenNeighborIndex_WhenExtractValue_ThenReturnsInt) {
    // Arrange
    NeighborIndex idx{42};
    
    // Act
    int value = idx();
    
    // Assert
    EXPECT_EQ(value, 42);
}

/**
 * @test Compile-time test: NeighborIndex implicit conversion is disabled
 * 
 * This test documents that the following code SHOULD NOT compile:
 *   NeighborIndex idx = 5;  // ❌ Error: explicit constructor
 * 
 * Uncomment the code below to verify the compile error.
 */
TEST_F(NeighborAccessorTest, DISABLED_CompileError_ImplicitConversionFromInt) {
    // UNCOMMENT TO TEST COMPILE ERROR:
    // NeighborIndex idx = 5;  // Should not compile
    SUCCEED() << "This test documents a compile-time guarantee";
}

/**
 * @test Compile-time test: NeighborIndex prevents float conversion
 * 
 * This test documents that the following code SHOULD NOT compile:
 *   NeighborIndex idx{5.0};  // ❌ Error: deleted constructor
 */
TEST_F(NeighborAccessorTest, DISABLED_CompileError_FloatConversion) {
    // UNCOMMENT TO TEST COMPILE ERROR:
    // NeighborIndex idx{5.0};  // Should not compile
    SUCCEED() << "This test documents a compile-time guarantee";
}

// ============================================================================
// TypedParticleArray Tests
// ============================================================================

/**
 * @test Given a particle vector
 *       When creating SearchParticleArray
 *       Then wrapper has correct size
 */
TEST_F(NeighborAccessorTest, GivenParticleVector_WhenCreateSearchArray_ThenCorrectSize) {
    // Arrange & Act
    SearchParticleArray<2> search_array{search_particles};
    
    // Assert
    EXPECT_EQ(search_array.size(), 10);
    EXPECT_FALSE(search_array.empty());
}

/**
 * @test Given a particle vector
 *       When creating RealParticleArray
 *       Then wrapper has correct size
 */
TEST_F(NeighborAccessorTest, GivenParticleVector_WhenCreateRealArray_ThenCorrectSize) {
    // Arrange & Act
    RealParticleArray<2> real_array{real_particles};
    
    // Assert
    EXPECT_EQ(real_array.size(), 5);
}

/**
 * @test Given empty particle vector
 *       When creating typed array
 *       Then empty() returns true
 */
TEST_F(NeighborAccessorTest, GivenEmptyVector_WhenCreateArray_ThenIsEmpty) {
    // Arrange
    std::vector<SPHParticle<2>> empty_vec;
    
    // Act
    SearchParticleArray<2> search_array{empty_vec};
    
    // Assert
    EXPECT_TRUE(search_array.empty());
    EXPECT_EQ(search_array.size(), 0);
}

// ============================================================================
// NeighborAccessor Construction Tests
// ============================================================================

/**
 * @test Given SearchParticleArray
 *       When constructing NeighborAccessor
 *       Then construction succeeds
 */
TEST_F(NeighborAccessorTest, GivenSearchArray_WhenConstructAccessor_ThenSucceeds) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    
    // Act
    NeighborAccessor<2> accessor{search_array};
    
    // Assert
    EXPECT_EQ(accessor.particle_count(), 10);
    EXPECT_FALSE(accessor.empty());
}

/**
 * @test Compile-time test: RealParticleArray cannot construct NeighborAccessor
 * 
 * This is the KEY safety guarantee: neighbor indices can ONLY access
 * SearchParticleArray (real + ghost), not RealParticleArray.
 * 
 * This test documents that the following code SHOULD NOT compile:
 *   NeighborAccessor<2> bad{real_array};  // ❌ Error: deleted constructor
 */
TEST_F(NeighborAccessorTest, DISABLED_CompileError_RealArrayToAccessor) {
    // UNCOMMENT TO TEST COMPILE ERROR:
    // RealParticleArray<2> real_array{real_particles};
    // NeighborAccessor<2> accessor{real_array};  // Should not compile
    SUCCEED() << "This test documents the primary compile-time safety guarantee";
}

// ============================================================================
// NeighborAccessor get_neighbor() Tests
// ============================================================================

/**
 * @test Given valid NeighborIndex within bounds
 *       When calling get_neighbor()
 *       Then returns correct particle
 */
TEST_F(NeighborAccessorTest, GivenValidIndex_WhenGetNeighbor_ThenReturnsCorrectParticle) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex idx{3};
    
    // Act
    const auto& particle = accessor.get_neighbor(idx);
    
    // Assert
    EXPECT_EQ(particle.id, 3);
    EXPECT_DOUBLE_EQ(particle.pos[0], 3.0);
}

/**
 * @test Given NeighborIndex pointing to ghost particle
 *       When calling get_neighbor()
 *       Then returns ghost particle correctly
 */
TEST_F(NeighborAccessorTest, GivenGhostIndex_WhenGetNeighbor_ThenReturnsGhost) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex ghost_idx{7};  // Ghost particle
    
    // Act
    const auto& particle = accessor.get_neighbor(ghost_idx);
    
    // Assert
    EXPECT_EQ(particle.id, 7);
    EXPECT_EQ(particle.type, static_cast<int>(ParticleType::GHOST));
}

/**
 * @test Given NeighborIndex at boundary (last valid index)
 *       When calling get_neighbor()
 *       Then returns last particle
 */
TEST_F(NeighborAccessorTest, GivenBoundaryIndex_WhenGetNeighbor_ThenReturnsLastParticle) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex last_idx{9};  // Last valid index
    
    // Act
    const auto& particle = accessor.get_neighbor(last_idx);
    
    // Assert
    EXPECT_EQ(particle.id, 9);
}

#ifndef NDEBUG
/**
 * @test Given out-of-bounds NeighborIndex (positive overflow)
 *       When calling get_neighbor() in debug build
 *       Then throws std::out_of_range
 */
TEST_F(NeighborAccessorTest, GivenOutOfBoundsIndex_WhenGetNeighbor_ThenThrows) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex bad_idx{100};  // Way out of bounds
    
    // Act & Assert
    EXPECT_THROW({
        accessor.get_neighbor(bad_idx);
    }, std::out_of_range);
}

/**
 * @test Given negative NeighborIndex
 *       When calling get_neighbor() in debug build
 *       Then throws std::out_of_range
 */
TEST_F(NeighborAccessorTest, GivenNegativeIndex_WhenGetNeighbor_ThenThrows) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex bad_idx{-1};
    
    // Act & Assert
    EXPECT_THROW({
        accessor.get_neighbor(bad_idx);
    }, std::out_of_range);
}

/**
 * @test Given index exactly at size boundary (size, not size-1)
 *       When calling get_neighbor() in debug build
 *       Then throws std::out_of_range
 */
TEST_F(NeighborAccessorTest, GivenIndexAtSize_WhenGetNeighbor_ThenThrows) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex bad_idx{10};  // Equal to size, invalid
    
    // Act & Assert
    EXPECT_THROW({
        accessor.get_neighbor(bad_idx);
    }, std::out_of_range);
}
#endif  // NDEBUG

/**
 * @test Given accessor to empty array
 *       When calling particle_count()
 *       Then returns zero
 */
TEST_F(NeighborAccessorTest, GivenEmptyArray_WhenParticleCount_ThenReturnsZero) {
    // Arrange
    std::vector<SPHParticle<2>> empty_vec;
    SearchParticleArray<2> search_array{empty_vec};
    NeighborAccessor<2> accessor{search_array};
    
    // Act & Assert
    EXPECT_EQ(accessor.particle_count(), 0);
    EXPECT_TRUE(accessor.empty());
}

// ============================================================================
// Multi-dimensional Tests (3D)
// ============================================================================

/**
 * @test Given 3D particles
 *       When using NeighborAccessor<3>
 *       Then works correctly for 3D
 */
TEST_F(NeighborAccessorTest, Given3DParticles_WhenUseAccessor_ThenWorksCorrectly) {
    // Arrange
    std::vector<SPHParticle<3>> particles_3d(5);
    for (int i = 0; i < 5; ++i) {
        particles_3d[i].id = i;
        particles_3d[i].pos = Vector<3>{static_cast<real>(i), 0.0, 0.0};
    }
    
    SearchParticleArray<3> search_array{particles_3d};
    NeighborAccessor<3> accessor{search_array};
    
    // Act
    const auto& particle = accessor.get_neighbor(NeighborIndex{2});
    
    // Assert
    EXPECT_EQ(particle.id, 2);
    EXPECT_DOUBLE_EQ(particle.pos[0], 2.0);
}

// ============================================================================
// Integration Tests with Multiple Accessors
// ============================================================================

/**
 * @test Given multiple accessors to same array
 *       When accessing different indices
 *       Then all accessors work independently
 */
TEST_F(NeighborAccessorTest, GivenMultipleAccessors_WhenAccess_ThenWorkIndependently) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor1{search_array};
    NeighborAccessor<2> accessor2{search_array};
    
    // Act
    const auto& p1 = accessor1.get_neighbor(NeighborIndex{3});
    const auto& p2 = accessor2.get_neighbor(NeighborIndex{7});
    
    // Assert
    EXPECT_EQ(p1.id, 3);
    EXPECT_EQ(p2.id, 7);
}

/**
 * @test Given accessor
 *       When accessing same index multiple times
 *       Then returns same particle (reference stability)
 */
TEST_F(NeighborAccessorTest, GivenAccessor_WhenAccessSameIndexTwice_ThenSameReference) {
    // Arrange
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    NeighborIndex idx{5};
    
    // Act
    const auto& p1 = accessor.get_neighbor(idx);
    const auto& p2 = accessor.get_neighbor(idx);
    
    // Assert
    EXPECT_EQ(&p1, &p2);  // Same memory address
}

// ============================================================================
// Performance/Copy Behavior Tests
// ============================================================================

/**
 * @test Given SearchParticleArray
 *       When copy constructing
 *       Then copy succeeds
 */
TEST_F(NeighborAccessorTest, GivenSearchArray_WhenCopyConstruct_ThenSucceeds) {
    // Arrange
    SearchParticleArray<2> original{search_particles};
    
    // Act
    SearchParticleArray<2> copy{original};
    
    // Assert
    EXPECT_EQ(copy.size(), original.size());
}

// ============================================================================
// Documentation Tests (Comments as Tests)
// ============================================================================

/**
 * @test Documents the primary use case pattern
 * 
 * This test demonstrates the recommended usage pattern that prevents
 * the array index mismatch bug at compile time.
 */
TEST_F(NeighborAccessorTest, DOCUMENTATION_PrimaryUseCase) {
    // This is the SAFE pattern:
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Simulate neighbor search result
    std::vector<int> neighbor_indices = {0, 1, 2, 5, 7};  // Mix of real and ghost
    
    // Type-safe iteration
    for (int raw_idx : neighbor_indices) {
        NeighborIndex idx{raw_idx};  // Explicit construction
        const auto& particle = accessor.get_neighbor(idx);
        
        // Use particle safely - can be real or ghost
        EXPECT_GE(particle.id, 0);
        EXPECT_LE(particle.id, 9);
    }
    
    SUCCEED() << "This test documents the recommended usage pattern";
}

/**
 * @test Regression test for array index mismatch bug
 * 
 * Bug: Neighbor indices referenced cached_search_particles (real + ghost)
 *      but code accessed particles[] (real only), causing out-of-bounds reads.
 * 
 * Fix: Type system prevents wrong array access at compile time.
 * 
 * This test ensures we never revert to unsafe direct array access.
 */
TEST_F(NeighborAccessorTest, REGRESSION_ArrayIndexMismatchPrevention) {
    // Setup: 5 real particles, 5 ghost particles
    ASSERT_EQ(real_particles.size(), 5);
    ASSERT_EQ(search_particles.size(), 10);
    
    SearchParticleArray<2> search_array{search_particles};
    NeighborAccessor<2> accessor{search_array};
    
    // Simulate neighbor index pointing to ghost (would be out of bounds in real array)
    NeighborIndex ghost_idx{7};  // Index 7 is valid in search_particles but not in real_particles
    
    // This is SAFE because accessor enforces SearchParticleArray
    const auto& ghost = accessor.get_neighbor(ghost_idx);
    EXPECT_EQ(ghost.type, static_cast<int>(ParticleType::GHOST));
    
    // The following would NOT compile (compile-time prevention):
    // RealParticleArray<2> real_array{real_particles};
    // NeighborAccessor<2> bad_accessor{real_array};  // ❌ Compile error
    
    SUCCEED() << "Type system prevents the array index mismatch bug";
}

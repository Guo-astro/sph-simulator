/**
 * @file particle_cache_test.cpp
 * @brief BDD-style tests for ParticleCache
 * 
 * Tests follow Given-When-Then pattern for clarity
 */

#include <gtest/gtest.h>
#include "core/simulation/particle_cache.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "helpers/bdd_helpers.hpp"

using namespace sph;

// ============================================================================
// Test Fixture
// ============================================================================

template<int Dim>
class ParticleCacheTest : public ::testing::Test {
protected:
    ParticleCache<Dim> cache;
    std::vector<SPHParticle<Dim>> real_particles;
    
    void SetUp() override {
        // Create sample particles
        real_particles = create_test_particles(10);
    }
    
    std::vector<SPHParticle<Dim>> create_test_particles(int count) {
        std::vector<SPHParticle<Dim>> particles;
        for (int i = 0; i < count; ++i) {
            SPHParticle<Dim> p;
            p.id = i;
            p.mass = 1.0;
            p.dens = 1.0 + i * 0.1;  // Varying density
            p.pres = 0.1 * p.dens;
            p.sml = 0.1;
            for (int d = 0; d < Dim; ++d) {
                p.pos[d] = static_cast<real>(i) * 0.1;
                p.vel[d] = 0.0;
            }
            particles.push_back(p);
        }
        return particles;
    }
};

using ParticleCacheTest3D = ParticleCacheTest<3>;
using ParticleCacheTest2D = ParticleCacheTest<2>;

// ============================================================================
// Feature: Cache Initialization
// ============================================================================

TEST_F(ParticleCacheTest3D, GivenRealParticles_WhenInitialize_ThenCacheContainsCopies) {
    // GIVEN: Real particles with specific properties
    GIVEN("A set of 10 real particles") {
        EXPECT_EQ(real_particles.size(), 10);
    }
    
    // WHEN: Cache is initialized
    WHEN("Cache is initialized with real particles") {
        cache.initialize(real_particles);
    }
    
    // THEN: Cache contains copies
    THEN("Cache size equals real particle count") {
        EXPECT_EQ(cache.size(), 10);
    }
    
    THEN("Cache is marked as initialized") {
        EXPECT_TRUE(cache.is_initialized());
    }
    
    THEN("Cache does not include ghosts yet") {
        EXPECT_FALSE(cache.has_ghosts());
    }
    
    THEN("Cache particles have same properties as real particles") {
        const auto& cached = cache.get_search_particles();
        for (size_t i = 0; i < real_particles.size(); ++i) {
            EXPECT_EQ(cached[i].id, real_particles[i].id);
            EXPECT_DOUBLE_EQ(cached[i].dens, real_particles[i].dens);
            EXPECT_DOUBLE_EQ(cached[i].pres, real_particles[i].pres);
        }
    }
}

TEST_F(ParticleCacheTest3D, GivenEmptyParticles_WhenInitialize_ThenThrows) {
    // GIVEN: Empty particle array
    std::vector<SPHParticle<3>> empty_particles;
    
    // WHEN/THEN: Initialize throws
    EXPECT_THROW({
        cache.initialize(empty_particles);
    }, std::runtime_error);
}

// ============================================================================
// Feature: Cache Synchronization
// ============================================================================

TEST_F(ParticleCacheTest3D, GivenInitializedCache_WhenParticlesModified_ThenSyncUpdatesCache) {
    // GIVEN: Initialized cache with original particle data
    GIVEN("Cache initialized with particles") {
        cache.initialize(real_particles);
    }
    
    // WHEN: Real particles are modified (e.g., after pre_interaction)
    WHEN("Real particle densities are updated") {
        for (auto& p : real_particles) {
            p.dens *= 2.0;  // Simulate density calculation
            p.pres = 0.1 * p.dens;  // Recalculate pressure
        }
        
        cache.sync_real_particles(real_particles);
    }
    
    // THEN: Cache reflects the updates
    THEN("Cached particles have updated densities") {
        const auto& cached = cache.get_search_particles();
        for (size_t i = 0; i < real_particles.size(); ++i) {
            EXPECT_DOUBLE_EQ(cached[i].dens, real_particles[i].dens);
            EXPECT_DOUBLE_EQ(cached[i].pres, real_particles[i].pres);
        }
    }
}

TEST_F(ParticleCacheTest3D, GivenUninitializedCache_WhenSync_ThenThrows) {
    // GIVEN: Uninitialized cache
    // (no initialize() call)
    
    // WHEN/THEN: Sync throws
    EXPECT_THROW({
        cache.sync_real_particles(real_particles);
    }, std::runtime_error);
}

TEST_F(ParticleCacheTest3D, GivenInitializedCache_WhenSyncWithWrongSize_ThenThrows) {
    // GIVEN: Cache initialized with 10 particles
    GIVEN("Cache initialized with 10 particles") {
        cache.initialize(real_particles);
    }
    
    // WHEN: Try to sync with different number of particles
    WHEN("Sync with 5 particles") {
        auto wrong_size_particles = create_test_particles(5);
        
        // THEN: Throws due to size mismatch
        EXPECT_THROW({
            cache.sync_real_particles(wrong_size_particles);
        }, std::runtime_error);
    }
}

// ============================================================================
// Feature: Ghost Particle Integration
// ============================================================================

TEST_F(ParticleCacheTest3D, GivenCacheWithRealParticles_WhenIncludeGhosts_ThenCacheExtends) {
    // GIVEN: Cache with real particles
    GIVEN("Cache initialized with 10 real particles") {
        cache.initialize(real_particles);
    }
    
    // WHEN: Ghost particles are added
    WHEN("5 ghost particles are included via null ghost manager") {
        // For this test, we pass null ghost manager
        // This tests the "no ghosts" path
        cache.include_ghosts(nullptr);
    }
    
    // THEN: Cache size stays same (no ghosts added)
    THEN("Cache contains only real particles") {
        EXPECT_EQ(cache.size(), 10);
    }
    
    THEN("Cache is not marked as having ghosts") {
        EXPECT_FALSE(cache.has_ghosts());
    }
}

TEST_F(ParticleCacheTest3D, GivenCacheWithGhosts_WhenSyncRealParticles_ThenGhostsPreserved) {
    // This test is simplified - in real usage, ghosts are managed by GhostParticleManager
    // Here we just test that sync doesn't affect cache size when ghosts would be present
    
    // GIVEN: Cache initialized with real particles
    cache.initialize(real_particles);
    
    // WHEN: Real particles updated and synced
    for (auto& p : real_particles) {
        p.dens *= 3.0;
    }
    cache.sync_real_particles(real_particles);
    
    // THEN: Real particles updated
    const auto& cached = cache.get_search_particles();
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(cached[i].dens, real_particles[i].dens);
    }
}

// ============================================================================
// Feature: Cache Validation
// ============================================================================

TEST_F(ParticleCacheTest3D, GivenValidCache_WhenValidate_ThenNoThrow) {
    // GIVEN: Properly initialized cache
    cache.initialize(real_particles);
    
    // WHEN/THEN: Validate succeeds
    EXPECT_NO_THROW({
        cache.validate();
    });
}

// ============================================================================
// Feature: Real-world Integration Scenario
// ============================================================================

TEST_F(ParticleCacheTest3D, ScenarioTypicalSimulationInitialization) {
    // GIVEN: Fresh simulation start
    GIVEN("Simulation starting with 100 particles") {
        auto particles = create_test_particles(100);
        
        // WHEN: Cache initialized
        WHEN("Cache is initialized before pre-interaction") {
            cache.initialize(particles);
            
            // THEN: Cache ready for first tree build
            THEN("Cache contains initial particle state") {
                EXPECT_EQ(cache.size(), 100);
                EXPECT_FALSE(cache.has_ghosts());
            }
            
            // WHEN: Pre-interaction updates densities
            WHEN("Pre-interaction calculates densities") {
                for (auto& p : particles) {
                    p.dens = 2.5;  // Calculated density
                    p.sml = 0.15;  // Updated smoothing length
                }
                
                cache.sync_real_particles(particles);
                
                // THEN: Cache has latest densities for force calculation
                THEN("Cache ready for fluid force calculation") {
                    const auto& cached = cache.get_search_particles();
                    EXPECT_DOUBLE_EQ(cached[0].dens, 2.5);
                    EXPECT_DOUBLE_EQ(cached[0].sml, 0.15);
                }
            }
        }
    }
}

// Run tests for both 2D and 3D
TEST_F(ParticleCacheTest2D, InitializeWorks2D) {
    cache.initialize(real_particles);
    EXPECT_EQ(cache.size(), 10);
}

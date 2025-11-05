// Use 1D for tests by default
static constexpr int Dim = 1;

#include <gtest/gtest.h>
#include "core/ghost_particle_manager.hpp"
#include "core/boundary_types.hpp"
#include "core/sph_particle.hpp"
#include <vector>
#include <cmath>

using namespace sph;

/**
 * Test basic ghost particle manager functionality
 */
class GhostParticleManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }
    
    // Helper to count ghosts with specific position in a dimension
    template<int Dim>
    int countGhostsInRange(const GhostParticleManager<Dim>& manager, 
                          int dim, real min_val, real max_val) {
        int count = 0;
        const auto& ghosts = manager.get_ghost_particles();
        for (const auto& ghost : ghosts) {
            if (ghost.pos[dim] >= min_val && ghost.pos[dim] <= max_val) {
                count++;
            }
        }
        return count;
    }
};

/**
 * Test 1D periodic boundary ghost generation
 */
TEST_F(GhostParticleManagerTest, Periodic1D_Basic) {
    // Create configuration for 1D periodic boundaries
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{0.0};
    config.range_max = Vector<1>{1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    // Create test particles near boundaries
    std::vector<SPHParticle<1>> particles;
    
    // Particle near lower boundary (should create ghost at upper side)
    SPHParticle<1> p1;
    p1.presos = Vector<1>{0.05};
    p1.vel = Vector<1>{1.0};
    p1.dens = 1.0;
    p1.pres = 1.0;
    p1.mass = 1.0;
    p1.sml = 0.05;
    particles.push_back(p1);
    
    // Particle near upper boundary (should create ghost at lower side)
    SPHParticle<1> p2;
    p2.presos = Vector<1>{0.95};
    p2.vel = Vector<1>{-1.0};
    p2.dens = 1.0;
    p2.pres = 1.0;
    p2.mass = 1.0;
    p2.sml = 0.05;
    particles.push_back(p2);
    
    // Particle in the middle (no ghosts)
    SPHParticle<1> p3;
    p3.presos = Vector<1>{0.5};
    p3.vel = Vector<1>{0.0};
    p3.dens = 1.0;
    p3.pres = 1.0;
    p3.mass = 1.0;
    p3.sml = 0.05;
    particles.push_back(p3);
    
    // Generate ghosts
    manager.generate_ghosts(particles);
    
    // Should create 2 ghosts (one for each boundary particle)
    EXPECT_EQ(manager.get_ghost_count(), 2);
    EXPECT_TRUE(manager.has_ghosts());
    
    // Verify ghost positions
    const auto& ghosts = manager.get_ghost_particles();
    ASSERT_EQ(ghosts.size(), 2);
    
    // Ghost from p1 should be at ~1.05
    EXPECT_NEAR(ghosts[0].pos[0], 1.05, 1e-10);
    EXPECT_NEAR(ghosts[0].vel[0], 1.0, 1e-10);
    
    // Ghost from p2 should be at ~-0.05
    EXPECT_NEAR(ghosts[1].pos[0], -0.05, 1e-10);
    EXPECT_NEAR(ghosts[1].vel[0], -1.0, 1e-10);
}

/**
 * Test 2D periodic boundary corner generation
 */
TEST_F(GhostParticleManagerTest, Periodic2D_Corners) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.types[1] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{0.0, 0.0};
    config.range_max = Vector<1>{1.0, 1.0};
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    // Create particle near corner (both x and y boundaries)
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.r = Vector<1>{0.05, 0.05};  // Near lower-left corner
    p.v = Vector<1>{1.0, 1.0};
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    // Should create:
    // - 1 ghost in +x direction (from x-periodic)
    // - 1 ghost in +y direction (from y-periodic)
    // - 1 ghost in +x,+y direction (corner)
    // Total: 3 ghosts
    EXPECT_EQ(manager.get_ghost_count(), 3);
    
    const auto& ghosts = manager.get_ghost_particles();
    
    // Verify we have ghosts in all expected positions
    bool has_x_ghost = false;
    bool has_y_ghost = false;
    bool has_corner_ghost = false;
    
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[0] - 1.05) < 1e-6 && std::abs(ghost.pos[1] - 0.05) < 1e-6) {
            has_x_ghost = true;
        }
        if (std::abs(ghost.pos[0] - 0.05) < 1e-6 && std::abs(ghost.pos[1] - 1.05) < 1e-6) {
            has_y_ghost = true;
        }
        if (std::abs(ghost.pos[0] - 1.05) < 1e-6 && std::abs(ghost.pos[1] - 1.05) < 1e-6) {
            has_corner_ghost = true;
        }
    }
    
    EXPECT_TRUE(has_x_ghost) << "Missing +x ghost";
    EXPECT_TRUE(has_y_ghost) << "Missing +y ghost";
    EXPECT_TRUE(has_corner_ghost) << "Missing corner ghost";
}

/**
 * Test 2D mixed boundaries (periodic x, mirror y)
 */
TEST_F(GhostParticleManagerTest, Mixed2D_PeriodicAndMirror) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;  // x: periodic
    config.types[1] = BoundaryType::MIRROR;     // y: mirror
    config.enable_lower[1] = true;
    config.enable_upper[1] = true;
    config.mirror_types[1] = MirrorType::NO_SLIP;
    config.range_min = Vector<1>{0.0, 0.0};
    config.range_max = Vector<1>{1.0, 1.0};
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    // Create test particle near y lower boundary
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.r = Vector<1>{0.5, 0.05};
    p.v = Vector<1>{1.0, 0.5};  // vx=1, vy=0.5
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    // Should create at least 1 ghost (for y mirror boundary)
    EXPECT_GT(manager.get_ghost_count(), 0);
    
    // Find the mirror ghost
    const auto& ghosts = manager.get_ghost_particles();
    bool found_mirror = false;
    for (const auto& ghost : ghosts) {
        // Mirror ghost should be at y=-0.05 with reflected velocity
        if (std::abs(ghost.pos[1] - (-0.05)) < 1e-6) {
            found_mirror = true;
            // Check velocity reflection (no-slip: all components reflected)
            EXPECT_NEAR(ghost.vel[0], -1.0, 1e-6) << "x-velocity should be reflected";
            EXPECT_NEAR(ghost.vel[1], -0.5, 1e-6) << "y-velocity should be reflected";
        }
    }
    
    EXPECT_TRUE(found_mirror) << "Should have a mirror ghost";
}

/**
 * Test mirror boundary with free-slip
 */
TEST_F(GhostParticleManagerTest, Mirror_FreeSlip) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::NONE;
    config.types[1] = BoundaryType::MIRROR;
    config.enable_lower[1] = true;
    config.enable_upper[1] = false;
    config.mirror_types[1] = MirrorType::FREE_SLIP;
    config.range_min = Vector<1>{0.0, 0.0};
    config.range_max = Vector<1>{1.0, 1.0};
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.r = Vector<1>{0.5, 0.05};
    p.v = Vector<1>{1.0, 0.5};  // tangential=1.0, normal=0.5
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    EXPECT_GT(manager.get_ghost_count(), 0);
    
    // Find mirror ghost and check free-slip velocity
    const auto& ghosts = manager.get_ghost_particles();
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.pos[1] - (-0.05)) < 1e-6) {
            // Free-slip: tangential velocity preserved, normal reflected
            EXPECT_NEAR(ghost.vel[0], 1.0, 1e-6) << "Tangential velocity preserved";
            EXPECT_NEAR(ghost.vel[1], -0.5, 1e-6) << "Normal velocity reflected";
        }
    }
}

/**
 * Test ghost update mechanism
 */
TEST_F(GhostParticleManagerTest, UpdateGhosts) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{0.0};
    config.range_max = Vector<1>{1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.r = Vector<1>{0.05};
    p.v = Vector<1>{1.0};
    p.dens = 1.0;
    p.p = 1.0;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    EXPECT_EQ(manager.get_ghost_count(), 1);
    
    // Modify real particle
    particles[0].v = Vector<1>{2.0};
    particles[0].dens = 2.0;
    
    // Update ghosts
    manager.update_ghosts(particles);
    
    // Ghost should now have updated properties
    const auto& ghosts = manager.get_ghost_particles();
    EXPECT_NEAR(ghosts[0].vel[0], 2.0, 1e-10);
    EXPECT_NEAR(ghosts[0].dens, 2.0, 1e-10);
}

/**
 * Test periodic wrapping
 */
TEST_F(GhostParticleManagerTest, PeriodicWrapping) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{0.0};
    config.range_max = Vector<1>{1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    std::vector<SPHParticle<1>> particles;
    
    // Particle that moved outside lower boundary
    SPHParticle<1> p1;
    p1.presos = Vector<1>{-0.1};
    particles.push_back(p1);
    
    // Particle that moved outside upper boundary
    SPHParticle<1> p2;
    p2.presos = Vector<1>{1.1};
    particles.push_back(p2);
    
    manager.apply_periodic_wrapping(particles);
    
    // Should be wrapped back
    EXPECT_NEAR(particles[0].pos[0], 0.9, 1e-10);  // -0.1 + 1.0
    EXPECT_NEAR(particles[1].pos[0], 0.1, 1e-10);  // 1.1 - 1.0
}

/**
 * Test boundary type string conversions
 */
TEST(BoundaryTypesTest, StringConversions) {
    EXPECT_EQ(string_to_boundary_type("periodic"), BoundaryType::PERIODIC);
    EXPECT_EQ(string_to_boundary_type("mirror"), BoundaryType::MIRROR);
    EXPECT_EQ(string_to_boundary_type("none"), BoundaryType::NONE);
    
    EXPECT_EQ(boundary_type_to_string(BoundaryType::PERIODIC), "periodic");
    EXPECT_EQ(boundary_type_to_string(BoundaryType::MIRROR), "mirror");
    
    EXPECT_EQ(string_to_mirror_type("no_slip"), MirrorType::NO_SLIP);
    EXPECT_EQ(string_to_mirror_type("free_slip"), MirrorType::FREE_SLIP);
}

/**
 * Test boundary configuration helpers
 */
TEST(BoundaryConfigurationTest, Helpers) {
    BoundaryConfiguration<2> config;
    config.types[0] = BoundaryType::PERIODIC;
    config.types[1] = BoundaryType::MIRROR;
    
    EXPECT_TRUE(config.has_periodic());
    EXPECT_TRUE(config.has_mirror());
    
    config.range_min = Vector<1>{0.0, 0.0};
    config.range_max = Vector<1>{1.0, 2.0};
    
    EXPECT_DOUBLE_EQ(config.get_range(0), 1.0);
    EXPECT_DOUBLE_EQ(config.get_range(1), 2.0);
}

/**
 * Test 3D periodic corners
 */
TEST_F(GhostParticleManagerTest, Periodic3D_Corners) {
    BoundaryConfiguration<3> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.types[1] = BoundaryType::PERIODIC;
    config.types[2] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{0.0, 0.0, 0.0};
    config.range_max = Vector<1>{1.0, 1.0, 1.0};
    
    GhostParticleManager<3> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    // Particle near corner (all three boundaries)
    std::vector<SPHParticle<3>> particles;
    SPHParticle<3> p;
    p.r = Vector<1>{0.05, 0.05, 0.05};
    p.v = Vector<1>{1.0, 1.0, 1.0};
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    // Should create multiple ghosts for faces, edges, and corner
    // Minimum: 3 face ghosts + 3 edge ghosts + 1 corner ghost = 7
    EXPECT_GE(manager.get_ghost_count(), 7);
}

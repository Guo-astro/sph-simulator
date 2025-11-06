// Use 1D for tests by default
static constexpr int Dim = 1;

#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/particles/sph_particle.hpp"
#include <iostream>
#include <vector>
#include <cmath>

using namespace sph;

// Simple test framework
int total_tests = 0;
int passed_tests = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        total_tests++; \
        std::cout << "Running test: " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASSED" << std::endl; \
            passed_tests++; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << std::endl; \
        } \
    } \
    void test_##name()

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Expected ") + std::to_string(a) + " == " + std::to_string(b)); \
    }

#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Expected true: ") + #cond); \
    }

#define ASSERT_NEAR(a, b, tol) \
    if (std::abs((a) - (b)) > (tol)) { \
        throw std::runtime_error(std::string("Expected ") + std::to_string(a) + " near " + std::to_string(b)); \
    }

TEST(Periodic1D_Basic) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = {0.0};
    config.range_max = {1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<1>> particles;
    
    // Particle near lower boundary
    SPHParticle<1> p1;
    p1.r = {0.05};
    p1.v = {1.0};
    p1.dens = 1.0;
    p1.p = 1.0;
    p1.m = 1.0;
    p1.h = 0.05;
    particles.push_back(p1);
    
    // Particle near upper boundary
    SPHParticle<1> p2;
    p2.r = {0.95};
    p2.v = {-1.0};
    p2.dens = 1.0;
    p2.p = 1.0;
    p2.m = 1.0;
    p2.h = 0.05;
    particles.push_back(p2);
    
    manager.generate_ghosts(particles);
    
    ASSERT_EQ(manager.get_ghost_count(), 2);
    ASSERT_TRUE(manager.has_ghosts());
    
    const auto& ghosts = manager.get_ghost_particles();
    ASSERT_NEAR(ghosts[0].r[0], 1.05, 1e-10);
    ASSERT_NEAR(ghosts[1].r[0], -0.05, 1e-10);
}

TEST(Periodic2D_Corners) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.types[1] = BoundaryType::PERIODIC;
    config.range_min = {0.0, 0.0};
    config.range_max = {1.0, 1.0};
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.r = {0.05, 0.05};
    p.v = {1.0, 1.0};
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    ASSERT_EQ(manager.get_ghost_count(), 3);
}

TEST(Mirror_NoSlip) {
    BoundaryConfiguration<2> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::NONE;
    config.types[1] = BoundaryType::MIRROR;
    config.enable_lower[1] = true;
    config.enable_upper[1] = false;
    config.mirror_types[1] = MirrorType::NO_SLIP;
    config.range_min = {0.0, 0.0};
    config.range_max = {1.0, 1.0};
    
    GhostParticleManager<2> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.1);
    
    std::vector<SPHParticle<2>> particles;
    SPHParticle<2> p;
    p.r = {0.5, 0.05};
    p.v = {1.0, 0.5};
    p.dens = 1.0;
    p.p = 1.0;
    p.m = 1.0;
    p.h = 0.05;
    particles.push_back(p);
    
    manager.generate_ghosts(particles);
    
    ASSERT_TRUE(manager.get_ghost_count() > 0);
    
    const auto& ghosts = manager.get_ghost_particles();
    for (const auto& ghost : ghosts) {
        if (std::abs(ghost.r[1] - (-0.05)) < 1e-6) {
            ASSERT_NEAR(ghost.v[0], -1.0, 1e-6);
            ASSERT_NEAR(ghost.v[1], -0.5, 1e-6);
        }
    }
}

TEST(PeriodicWrapping) {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = {0.0};
    config.range_max = {1.0};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    std::vector<SPHParticle<1>> particles;
    
    SPHParticle<1> p1;
    p1.r = {-0.1};
    particles.push_back(p1);
    
    SPHParticle<1> p2;
    p2.r = {1.1};
    particles.push_back(p2);
    
    manager.apply_periodic_wrapping(particles);
    
    ASSERT_NEAR(particles[0].r[0], 0.9, 1e-10);
    ASSERT_NEAR(particles[1].r[0], 0.1, 1e-10);
}

int main() {
    std::cout << "\n=== Ghost Particle Manager Tests ===\n\n";
    
    run_test_Periodic1D_Basic();
    run_test_Periodic2D_Corners();
    run_test_Mirror_NoSlip();
    run_test_PeriodicWrapping();
    
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Total tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";
    
    return (total_tests == passed_tests) ? 0 : 1;
}

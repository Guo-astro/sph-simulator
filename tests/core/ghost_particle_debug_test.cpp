// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file ghost_particle_debug_test.cpp
 * @brief Debug tests to diagnose the actual ghost particle issues
 */

#include <gtest/gtest.h>
#include "core/ghost_particle_manager.hpp"
#include "core/sph_particle.hpp"
#include "core/boundary_types.hpp"
#include "defines.hpp"
#include <iostream>

using namespace sph;

TEST(GhostParticleDebugTest, ShockTubeSetup_DetailedInspection) {
    // Exact shock tube setup from user's simulation
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    
    // Typical smoothing length for 100 particles over length 2.0
    real h = 2.0 / 100.0;  // = 0.02
    real kernel_support = 2.0 * h;  // = 0.04
    manager.set_kernel_support_radius(kernel_support);
    
    std::cout << "\n=== Shock Tube Configuration ===\n";
    std::cout << "Domain: [" << config.range_min[0] << ", " << config.range_max[0] << "]\n";
    std::cout << "Smoothing length h = " << h << "\n";
    std::cout << "Kernel support radius = " << kernel_support << "\n\n";
    
    // Test particles at various positions near boundaries
    std::vector<real> test_positions = {
        -0.5,      // Exactly at left boundary
        -0.49,     // 0.01 inside left boundary
        -0.48,     // 0.02 inside (within kernel support!)
        -0.46,     // 0.04 inside (exactly at kernel support)
        -0.45,     // 0.05 inside (just outside kernel support)
        1.45,      // 0.05 inside right boundary
        1.46,      // 0.04 inside right boundary (exactly at kernel support)
        1.48,      // 0.02 inside right boundary (within kernel support!)
        1.49,      // 0.01 inside right boundary
        1.5        // Exactly at right boundary
    };
    
    std::vector<SPHParticle<1>> real_particles;
    for (real x : test_positions) {
        SPHParticle<1> p;
        p.pos = Vector<1>{x};
        p.vel = Vector<1>{0.0};
        p.dens = 1.0;
        p.mass = 1.0;
        p.sml = h;
        p.type = static_cast<int>(ParticleType::REAL);
        real_particles.push_back(p);
    }
    
    // Generate ghosts
    manager.generate_ghosts(real_particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    std::cout << "=== Ghost Generation Results ===\n";
    std::cout << "Total ghosts created: " << ghosts.size() << "\n\n";
    
    std::cout << "Position Analysis:\n";
    for (size_t i = 0; i < real_particles.size(); ++i) {
        real x = real_particles[i].pos[0];
        real dist_left = x - config.range_min[0];
        real dist_right = config.range_max[0] - x;
        
        // Check if ghosts were created
        int ghost_count = 0;
        for (const auto& ghost : ghosts) {
            // Ghost from left boundary particle appears at right
            if (dist_left < kernel_support && std::abs(ghost.pos[0] - (x + 2.0)) < 1e-6) {
                ghost_count++;
            }
            // Ghost from right boundary particle appears at left  
            if (dist_right < kernel_support && std::abs(ghost.pos[0] - (x - 2.0)) < 1e-6) {
                ghost_count++;
            }
        }
        
        std::cout << "x=" << x 
                  << ", dist_left=" << dist_left 
                  << ", dist_right=" << dist_right
                  << ", ghosts=" << ghost_count;
        
        if (dist_left < kernel_support || dist_right < kernel_support) {
            std::cout << " [SHOULD CREATE GHOST]";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n=== Ghost Particle Details ===\n";
    for (size_t i = 0; i < ghosts.size(); ++i) {
        std::cout << "Ghost " << i << ": pos=" << ghosts[i].pos[0] 
                  << ", vel=" << ghosts[i].vel[0] << "\n";
    }
    
    // Critical tests
    EXPECT_GT(ghosts.size(), 0) << "Should create ghosts for particles near boundaries";
    
    // Count ghosts near each boundary region
    int ghosts_at_left = 0;
    int ghosts_at_right = 0;
    for (const auto& g : ghosts) {
        if (g.pos[0] < -0.4 && g.pos[0] > -0.6) ghosts_at_left++;
        if (g.pos[0] > 1.4 && g.pos[0] < 1.6) ghosts_at_right++;
    }
    
    std::cout << "\nGhosts near left boundary region: " << ghosts_at_left << "\n";
    std::cout << "Ghosts near right boundary region: " << ghosts_at_right << "\n";
    
    EXPECT_GT(ghosts_at_left, 0) << "Should have ghosts at left boundary";
    EXPECT_GT(ghosts_at_right, 0) << "Should have ghosts at right boundary";
}

TEST(GhostParticleDebugTest, IsNearBoundary_Logic) {
    // Test the is_near_boundary logic directly
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.04);
    
    std::cout << "\n=== Testing is_near_boundary Logic ===\n";
    std::cout << "Kernel support = 0.04\n";
    std::cout << "Lower boundary = -0.5\n";
    std::cout << "Upper boundary = 1.5\n\n";
    
    // We need to create a test harness since is_near_boundary is private
    // Instead, we'll test by checking ghost generation
    std::vector<SPHParticle<1>> particles;
    
    // Particle exactly at lower boundary
    SPHParticle<1> p1;
    p1.pos = Vector<1>{-0.5};
    p1.vel = Vector<1>{1.0};
    p1.dens = 1.0;
    p1.mass = 1.0;
    p1.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p1);
    
    manager.generate_ghosts(particles);
    auto ghosts = manager.get_ghost_particles();
    
    std::cout << "Particle at x=-0.5: ";
    if (ghosts.size() > 0) {
        std::cout << "CREATES " << ghosts.size() << " ghost(s)\n";
        for (const auto& g : ghosts) {
            std::cout << "  Ghost at x=" << g.pos[0] << "\n";
        }
    } else {
        std::cout << "NO GHOSTS CREATED [BUG!]\n";
    }
    
    EXPECT_GT(ghosts.size(), 0) << "Particle at boundary should create ghost!";
}

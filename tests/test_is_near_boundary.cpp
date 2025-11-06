// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file test_is_near_boundary.cpp
 * @brief Minimal test to debug is_near_boundary logic
 */

#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/particles/sph_particle.hpp"
#include <iostream>

using namespace sph;

int main() {
    BoundaryConfiguration<1> config;
    config.is_valid = true;
    config.types[0] = BoundaryType::PERIODIC;
    config.range_min = Vector<1>{-0.5};
    config.range_max = Vector<1>{1.5};
    
    GhostParticleManager<1> manager;
    manager.initialize(config);
    manager.set_kernel_support_radius(0.04);
    
    // Test single particle at x=1.46
    std::vector<SPHParticle<1>> particles;
    SPHParticle<1> p;
    p.pos = Vector<1>{1.46};
    p.vel = Vector<1>{0.0};
    p.dens = 1.0;
    p.mass = 1.0;
    p.type = static_cast<int>(ParticleType::REAL);
    particles.push_back(p);
    
    std::cout << "Testing particle at x=1.46\n";
    std::cout << "Upper boundary: 1.5\n";
    std::cout << "Distance: " << (1.5 - 1.46) << "\n";
    std::cout << "Kernel support: 0.04\n";
    std::cout << "Should create ghost: " << (0.04 <= 0.04 ? "YES" : "NO") << "\n\n";
    
    manager.generate_ghosts(particles);
    const auto& ghosts = manager.get_ghost_particles();
    
    std::cout << "Ghosts created: " << ghosts.size() << "\n";
    for (const auto& g : ghosts) {
        std::cout << "  Ghost at x=" << g.pos[0] << "\n";
    }
    
    return 0;
}

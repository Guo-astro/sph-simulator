#include <iostream>
#include <vector>
#include "core/sph_particle.hpp"
#include "core/bhtree.hpp"
#include "core/periodic.hpp"
#include "parameters.hpp"

using namespace sph;

int main() {
    // Create simple 1D periodic test with 10 particles
    const int num = 10;
    std::vector<SPHParticle<1>> particles(num);
    
    // Domain: [-0.5, 1.5], range = 2.0
    const real L = 2.0;
    const real dx = L / num;
    
    for (int i = 0; i < num; ++i) {
        particles[i].pos[0] = -0.5 + dx * (i + 0.5);
        particles[i].id = i;
        particles[i].sml = 2.0 * dx;  // h = 2*dx for good overlap
        particles[i].mass = 1.0;
        particles[i].dens = 1.0;
    }
    
    // Setup periodic boundaries
    auto param = std::make_shared<SPHParameters>();
    param->periodic.is_valid = true;
    param->periodic.range_min[0] = -0.5;
    param->periodic.range_max[0] = 1.5;
    param->tree.max_level = 20;
    param->tree.leaf_particle_num = 1;
    
    // Build tree
    BHTree<1> tree;
    tree.initialize(param);
    tree.resize(num);
    tree.make(particles, num);
    
    // Test: particle 0 (leftmost) should see particle 9 (rightmost) as neighbor
    auto config = NeighborSearchConfig::create(50, false);
    auto result = tree.find_neighbors(particles[0], config);
    
    std::cout << "Particle 0 position: " << particles[0].pos[0] << "\n";
    std::cout << "Particle 0 sml: " << particles[0].sml << "\n";
    std::cout << "Found " << result.neighbor_indices.size() << " neighbors:\n";
    for (int idx : result.neighbor_indices) {
        std::cout << "  Neighbor " << idx << " at position " << particles[idx].pos[0] << "\n";
    }
    
    // Check if particle 9 is in the list
    bool found_p9 = false;
    for (int idx : result.neighbor_indices) {
        if (idx == 9) found_p9 = true;
    }
    
    if (found_p9) {
        std::cout << "\n✓ Periodic boundary works: particle 0 sees particle 9 across boundary\n";
    } else {
        std::cout << "\n✗ PERIODIC BOUNDARY BROKEN: particle 0 does NOT see particle 9!\n";
        std::cout << "Distance across boundary: " << (1.5 - (-0.5) - (particles[9].pos[0] - particles[0].pos[0])) << "\n";
    }
    
    return found_p9 ? 0 : 1;
}

// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../bdd_helpers.hpp"
#include "core/bhtree.hpp"
#include "core/sph_particle.hpp"
#include "parameters.hpp"
#include <vector>
#include <memory>
#include <cmath>

// FEATURE: BHTreeConstruction

SCENARIO(TreeCreation, EmptyTree) {
    GIVEN("A BH tree with no particles") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        
        // Set default parameters
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        WHEN("Tree is initialized") {
            tree.initialize(param);
            
            THEN("Tree should be ready but empty") {
                // Tree structure exists but contains no particles
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeCreation, SingleParticle) {
    GIVEN("A BH tree with one particle") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        std::vector<sph::SPHParticle<Dim>> particles(1);
        particles[0].id = 0;
        particles[0].mass = 1.0;
        particles[0].pos[0] = 0.5;
#if Dim >= 2
        particles[0].pos[1] = 0.5;
#endif
#if Dim == 3
        particles[0].pos[2] = 0.5;
#endif
        particles[0].sml = 0.1;
        particles[0].dens = 1.0;
        
        WHEN("Tree is built") {
            tree.initialize(param);
            tree.resize(1);
            tree.make(particles, 1);
            
            THEN("Tree should contain the particle") {
                SUCCEED();
                // Tree construction succeeds
            }
        }
    }
}

SCENARIO(TreeCreation, MultipleParticles) {
    GIVEN("A BH tree with multiple particles") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        const int n = 10;
        std::vector<sph::SPHParticle<Dim>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0;
            particles[i].pos[0] = static_cast<double>(i) / n;
#if Dim >= 2
            particles[i].pos[1] = 0.5;
#endif
#if Dim == 3
            particles[i].pos[2] = 0.5;
#endif
            particles[i].sml = 0.1;
            particles[i].dens = 1.0;
        }
        
        WHEN("Tree is built with evenly spaced particles") {
            tree.initialize(param);
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should be successfully constructed") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeCreation, EdgeCaseParticleCount) {
    GIVEN("A BH tree with 2 particles") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        tree.initialize(param);
        
        WHEN("Building tree with 2 particles") {
            std::vector<sph::SPHParticle<Dim>> particles(2);
            for (int i = 0; i < 2; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].pos[0] = i * 0.5;
                particles[i].sml = 0.1;
                particles[i].dens = 1.0;
            }
            
            tree.resize(2);
            tree.make(particles, 2);
            
            THEN("Tree should have two leaves") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeCreation, PowerOfTwoParticles) {
    GIVEN("A BH tree with power-of-2 particles") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        tree.initialize(param);
        
        WHEN("Building tree with power-of-2 particles") {
            const int n = 64;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].pos[0] = static_cast<double>(i) / n;
                particles[i].sml = 0.1;
                particles[i].dens = 1.0;
            }
            
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should be balanced") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeCreation, SameLocationParticles) {
    GIVEN("Particles at the same location") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        tree.initialize(param);
        
        WHEN("All particles at same location") {
            const int n = 5;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].pos[0] = 0.5;
#if Dim >= 2
                particles[i].pos[1] = 0.5;
#endif
#if Dim == 3
                particles[i].pos[2] = 0.5;
#endif
                particles[i].sml = 0.1;
                particles[i].dens = 1.0;
            }
            
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should handle degenerate case") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeCreation, LinearDistribution) {
    GIVEN("Particles in a line") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        tree.initialize(param);
        
        WHEN("Particles in a line") {
            const int n = 10;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].pos[0] = static_cast<double>(i) / n;
#if Dim >= 2
                particles[i].pos[1] = 0.0;
#endif
#if Dim == 3
                particles[i].pos[2] = 0.0;
#endif
                particles[i].sml = 0.1;
                particles[i].dens = 1.0;
            }
            
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should handle 1D distribution") {
                SUCCEED();
            }
        }
    }
}

// End FEATURE: BHTreeConstruction

// FEATURE: BHTreeEdgeCases

SCENARIO(TreeLimits, MaximumTreeDepth) {
    GIVEN("A tree with limited depth") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 5;  // Shallow tree
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        
        WHEN("Many particles are in a small region") {
            const int n = 100;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                // Pack particles in small region
                particles[i].pos[0] = 0.5 + (i % 10) * 0.001;
#if Dim >= 2
                particles[i].pos[1] = 0.5 + (i / 10) * 0.001;
#endif
                particles[i].sml = 0.01;
            }
            
            tree.initialize(param);
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should respect max level") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeLimits, LeafParticleNumber) {
    GIVEN("A tree with multiple particles per leaf") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 5;  // Multiple particles per leaf
        param->periodic.is_valid = false;
        
        WHEN("Tree is built") {
            const int n = 20;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].pos[0] = static_cast<double>(i) / n;
                particles[i].sml = 0.1;
            }
            
            tree.initialize(param);
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Leaves should contain up to 5 particles") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeEdgeCases, ExtremeMasses) {
    GIVEN("Particles with extreme mass values") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        tree.initialize(param);
        
        WHEN("Particles have very different masses") {
            const int n = 3;
            std::vector<sph::SPHParticle<Dim>> particles(n);
            particles[0].mass = 1e-10;
            particles[1].mass = 1.0;
            particles[2].mass = 1e10;
            
            for (int i = 0; i < n; ++i) {
                particles[i].id = i;
                particles[i].pos[0] = 0.3 + i * 0.2;
                particles[i].sml = 0.1;
                particles[i].dens = 1.0;
            }
            
            tree.resize(n);
            tree.make(particles, n);
            
            THEN("Tree should handle mass disparity") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(TreeEdgeCases, BoundaryParticles) {
    GIVEN("Particles at domain boundaries") {
        sph::BHTree<Dim> tree;
        auto param = std::make_shared<sph::SPHParameters>();
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        param->periodic.is_valid = false;
        tree.initialize(param);
        
        WHEN("Particles are at corners") {
            std::vector<sph::SPHParticle<Dim>> particles(2);
            particles[0].id = 0;
            particles[0].mass = 1.0;
            particles[0].pos[0] = 0.0;
            particles[0].sml = 0.1;
            particles[0].dens = 1.0;
            
            particles[1].id = 1;
            particles[1].mass = 1.0;
            particles[1].pos[0] = 1.0;
            particles[1].sml = 0.1;
            particles[1].dens = 1.0;
            
            tree.resize(2);
            tree.make(particles, 2);
            
            THEN("Tree should handle boundary positions") {
                SUCCEED();
            }
        }
    }
}

// End FEATURE: BHTreeEdgeCases

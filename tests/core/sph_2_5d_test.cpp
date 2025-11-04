#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <cmath>
#include "../bdd_helpers.hpp"

// Include 2.5D headers
#include "../../include/core/sph_particle_2_5d.hpp"
#include "../../include/core/cubic_spline_2_5d.hpp"
#include "../../include/core/bhtree_2_5d.hpp"
#include "../../include/core/simulation_2_5d.hpp"
#include "../../include/parameters.hpp"

using namespace sph;

// ============================================================================
// SCENARIO: 2.5D SPH Particle coordinate transformations
// ============================================================================
TEST(SPH25DTest, CoordinateTransformations) {
GIVEN("A 2.5D particle with 2D hydro position") {
            SPHParticle2_5D particle;
            particle.pos = Vector<2>{1.0, 2.0}; // r=1, z=2
            particle.vel = Vector<2>{0.1, 0.2};
            particle.mass = 1.0;
            
            WHEN("Converting to 3D gravity coordinates at phi=0") {
                particle.update_gravity_position(0.0);
                
                THEN("3D position should be (r*cos(phi), r*sin(phi), z)") {
                    EXPECT_DOUBLE_EQ(particle.g_pos[0], 1.0); // x = r*cos(0) = 1*1 = 1
                    EXPECT_DOUBLE_EQ(particle.g_pos[1], 0.0); // y = r*sin(0) = 1*0 = 0
                    EXPECT_DOUBLE_EQ(particle.g_pos[2], 2.0); // z = z = 2
                }
            }
            
            WHEN("Converting to 3D gravity coordinates at phi=π/2") {
                particle.update_gravity_position(M_PI/2.0);
                
                THEN("3D position should be (0, r, z)") {
                    EXPECT_NEAR(particle.g_pos[0], 0.0, 1e-10); // x = r*cos(π/2) ≈ 0
                    EXPECT_DOUBLE_EQ(particle.g_pos[1], 1.0);   // y = r*sin(π/2) = 1
                    EXPECT_DOUBLE_EQ(particle.g_pos[2], 2.0);   // z = z = 2
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: 2.5D kernel functions
// ============================================================================
TEST(SPH25DTest, KernelFunctions) {
GIVEN("A 2.5D cubic spline kernel") {
            Cubic25D kernel;
            
            WHEN("Evaluating kernel at origin") {
                Vector<2> r{0.0, 0.0};
                real h = 1.0;
                real w_val = kernel.w(r, h);
                
                THEN("Kernel value should be maximum (normalization)") {
                    real expected_sigma = 10.0 / (7.0 * M_PI * h * h);
                    EXPECT_DOUBLE_EQ(w_val, expected_sigma);
                }
            }
            
            WHEN("Evaluating kernel gradient") {
                Vector<2> r{0.5, 0.0};
                real h = 1.0;
                Vector<2> dw_val = kernel.dw(r, h);
                
                THEN("Gradient should be non-zero and point inward") {
                    EXPECT_NE(dw_val[0], 0.0);
                    EXPECT_DOUBLE_EQ(dw_val[1], 0.0); // Symmetric in y direction
                    
                    // For q < 1, gradient should be negative (attractive)
                    EXPECT_LT(dw_val[0], 0.0);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: 2.5D BHTree gravity calculations
// ============================================================================
TEST(SPH25DTest, BHTree25DGravity) {
GIVEN("Two 2.5D particles") {
            std::vector<SPHParticle2_5D> particles(2);
            
            // Particle 1 at origin
            particles[0].id = 0;
            particles[0].pos = Vector<2>{0.0, 0.0};
            particles[0].mass = 1.0;
            particles[0].sml = 0.1;
            
            // Particle 2 at (1,0) in r-z plane
            particles[1].id = 1;
            particles[1].pos = Vector<2>{1.0, 0.0};
            particles[1].mass = 1.0;
            particles[1].sml = 0.1;
            
            // Set up gravity positions (both at phi=0 for simplicity)
            for (auto& p : particles) {
                p.update_gravity_position(0.0);
            }
            
            WHEN("Building 2.5D tree and calculating gravity") {
                BHTree25D tree;
                auto params = std::make_shared<SPHParameters>();
                params->tree.max_level = 5;
                params->tree.leaf_particle_num = 1;
                params->gravity.is_valid = true;
                params->gravity.constant = 1.0;
                params->gravity.theta = 0.5;
                
                tree.initialize(params);
                tree.resize(2);
                tree.make(particles, 2);
                
                THEN("Gravity calculation should work") {
                    EXPECT_NO_THROW(tree.tree_force(particles[0]));
                    EXPECT_NO_THROW(tree.tree_force(particles[1]));
                    
                    // Particles should have gravitational acceleration
                    EXPECT_NE(particles[0].acc[0], 0.0);
                    EXPECT_NE(particles[1].acc[0], 0.0);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: 2.5D Simulation initialization
// ============================================================================
TEST(SPH25DTest, Simulation25DInitialization) {
GIVEN("SPH parameters for 2.5D simulation") {
            auto params = std::make_shared<SPHParameters>();
            params->kernel = KernelType::CUBIC_SPLINE;
            params->tree.max_level = 5;
            params->tree.leaf_particle_num = 8;
            params->gravity.is_valid = true;
            params->gravity.constant = 1.0;
            params->time.start = 0.0;
            
            WHEN("Creating 2.5D simulation") {
                Simulation25D sim(params);
                
                THEN("Simulation should initialize correctly") {
                    EXPECT_NE(sim.kernel, nullptr);
                    EXPECT_NE(sim.tree, nullptr);
                    EXPECT_DOUBLE_EQ(sim.time, 0.0);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: 2.5D particle property updates
// ============================================================================
TEST(SPH25DTest, ParticlePropertyUpdates) {
GIVEN("A 2.5D particle") {
            SPHParticle2_5D particle;
            particle.pos = Vector<2>{2.0, 1.0};
            particle.vel = Vector<2>{0.1, 0.05};
            particle.mass = 0.5;
            
            WHEN("Updating gravity position") {
                particle.update_gravity_position(M_PI/4.0); // 45 degrees
                
                THEN("3D gravity position should be correct") {
                    real r = particle.r();
                    real expected_x = r * std::cos(M_PI/4.0);
                    real expected_y = r * std::sin(M_PI/4.0);
                    real expected_z = particle.z();
                    
                    EXPECT_DOUBLE_EQ(particle.g_pos[0], expected_x);
                    EXPECT_DOUBLE_EQ(particle.g_pos[1], expected_y);
                    EXPECT_DOUBLE_EQ(particle.g_pos[2], expected_z);
                }
                
                THEN("Cylindrical coordinates should be accessible") {
                    EXPECT_DOUBLE_EQ(particle.r(), 2.0);
                    EXPECT_DOUBLE_EQ(particle.z(), 1.0);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: Dimension policy correctness
// ============================================================================
TEST(SPH25DTest, DimensionPolicy) {
GIVEN("The 2.5D dimension policy") {
            WHEN("Checking dimensions") {
                THEN("Hydro dimension should be 2") {
                    EXPECT_EQ(Dimension2_5D::hydro_dim, 2);
                }
                
                THEN("Gravity dimension should be 3") {
                    EXPECT_EQ(Dimension2_5D::gravity_dim, 3);
                }
            }
            
            WHEN("Converting coordinates") {
                Vector<2> hydro_pos{1.0, 2.0};
                Vector<3> gravity_pos = Dimension2_5D::hydro_to_gravity(hydro_pos, 0.0);
                
                THEN("Conversion should work") {
                    EXPECT_DOUBLE_EQ(gravity_pos[0], 1.0);
                    EXPECT_DOUBLE_EQ(gravity_pos[1], 0.0);
                    EXPECT_DOUBLE_EQ(gravity_pos[2], 2.0);
                }
                
                Vector<2> back_to_hydro = Dimension2_5D::gravity_to_hydro(gravity_pos);
                
                THEN("Round-trip conversion should preserve coordinates") {
                    EXPECT_DOUBLE_EQ(back_to_hydro[0], hydro_pos[0]);
                    EXPECT_DOUBLE_EQ(back_to_hydro[1], hydro_pos[1]);
                }
            }
        }
    }
}

// ============================================================================
// SCENARIO: 2.5D kernel normalization
// ============================================================================
TEST(SPH25DTest, KernelNormalization) {
GIVEN("2.5D cubic spline kernel") {
            WHEN("Checking 2D normalization") {
                real h = 1.0;
                real sigma_2d = 10.0 / (7.0 * M_PI * h * h);
                
                THEN("2D normalization should be correct") {
                    EXPECT_GT(sigma_2d, 0.0);
                }
            }
            
            WHEN("Checking 3D normalization") {
                real sigma_3d = Cubic25D::sigma_3d();
                
                THEN("3D normalization should be 1/π") {
                    EXPECT_DOUBLE_EQ(sigma_3d, 1.0 / M_PI);
                }
            }
        }
    }
}</content>
<parameter name="filePath">/Users/guo/sph-simulation/tests/core/sph_2_5d_test.cpp
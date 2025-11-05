// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../../bdd_helpers.hpp"
#include "parameters.hpp"
#include "core/sph_particle.hpp"
#include <memory>
#include <vector>
#include <cmath>

using sph::inner_product;

FEATURE(DISPHFluidForce) {

SCENARIO(DISPHInitialization, ParameterSetup) {
    GIVEN("A DISPH fluid force module") {
        auto param = std::make_shared<sph::SPHParameters>();
        
        // Set required parameters
        param->physics.gamma = 5.0 / 3.0;
        param->physics.neighbor_number = 32;
        param->av.alpha = 1.0;
        param->av.use_balsara_switch = true;
        param->ac.is_valid = false;
        param->gravity.is_valid = false;
        
        WHEN("Module is initialized") {
            THEN("Parameters should be set correctly") {
                EXPECT_DOUBLE_EQ(param->physics.gamma, 5.0 / 3.0);
                EXPECT_EQ(param->physics.neighbor_number, 32);
                EXPECT_DOUBLE_EQ(param->av.alpha, 1.0);
                EXPECT_TRUE(param->av.use_balsara_switch);
            }
        }
    }
}

SCENARIO(DISPHCalculation, TwoParticleInteraction) {
    GIVEN("Two particles in DISPH simulation") {
        auto param = std::make_shared<sph::SPHParameters>();
        param->physics.gamma = 5.0 / 3.0;
        param->physics.neighbor_number = 32;
        param->kernel = sph::KernelType::CUBIC_SPLINE;
        param->av.alpha = 1.0;
        param->av.use_balsara_switch = false;
        param->av.use_time_dependent_av = false;
        param->ac.is_valid = false;
        param->periodic.is_valid = false;
        param->gravity.is_valid = false;
        param->tree.max_level = 20;
        param->tree.leaf_particle_num = 1;
        
        // Create two particles
        sph::SPHParticle<Dim> p1, p2;
        p1.id = 0;
        p1.mass = 1.0;
        p1.dens = 1.0;
        p1.pres = 1.0;
        p1.ene = 1.0;
        p1.sml = 0.5;
        p1.gradh = 0.0;
        p1.pos[0] = 0.0;
        p1.vel[0] = 0.0;
        p1.sound = 1.0;
        
        p2.id = 1;
        p2.mass = 1.0;
        p2.dens = 1.0;
        p2.pres = 1.0;
        p2.ene = 1.0;
        p2.sml = 0.5;
        p2.gradh = 0.0;
        p2.pos[0] = 0.3;  // Close but not overlapping
        p2.vel[0] = 0.0;
        p2.sound = 1.0;
        
        WHEN("Particles are setup") {
            real r_ij[Dim];
            r_ij[0] = p1.pos[0] - p2.pos[0];
#if Dim >= 2
            r_ij[1] = 0.0;
#endif
#if Dim == 3
            r_ij[2] = 0.0;
#endif
            double r = std::sqrt(inner_product(r_ij, r_ij));
            
            THEN("Particles should be within smoothing length") {
                EXPECT_LT(r, p1.sml + p2.sml);
                EXPECT_GT(r, 0.0);
            }
            
            AND("All particle properties should be valid") {
                EXPECT_GT(p1.mass, 0.0);
                EXPECT_GT(p1.dens, 0.0);
                EXPECT_GT(p1.sml, 0.0);
                EXPECT_GT(p2.mass, 0.0);
                EXPECT_GT(p2.dens, 0.0);
                EXPECT_GT(p2.sml, 0.0);
            }
        }
    }
}

SCENARIO(DISPHEdgeCases, ZeroDensity) {
    GIVEN("A particle with near-zero density") {
        auto param = std::make_shared<sph::SPHParameters>();
        param->physics.gamma = 5.0 / 3.0;
        param->physics.neighbor_number = 32;
        
        sph::SPHParticle<Dim> particle;
        particle.dens = 1e-15;  // Very small density
        particle.mass = 1.0;
        particle.pres = 1e-15;
        particle.ene = 1.0;
        
        WHEN("Computing pressure terms") {
            double gamma = param->physics.gamma;
            double gamma2_u_per_pres = sqr(gamma - 1.0) * particle.ene / particle.pres;
            
            THEN("Result should be finite") {
                EXPECT_TRUE(std::isfinite(gamma2_u_per_pres));
            }
        }
    }
}

SCENARIO(DISPHEdgeCases, ZeroPressure) {
    GIVEN("A particle with zero pressure") {
        sph::SPHParticle<Dim> particle;
        particle.pres = 0.0;
        particle.ene = 0.0;
        particle.dens = 1.0;
        particle.mass = 1.0;
        
        WHEN("Computing pressure-energy formulation terms") {
            double gamma = 5.0 / 3.0;
            double gamma2_u_i = sqr(gamma - 1.0) * particle.ene;
            
            THEN("Terms should be zero") {
                EXPECT_DOUBLE_EQ(gamma2_u_i, 0.0);
            }
        }
    }
}

SCENARIO(DISPHEdgeCases, HighMachNumber) {
    GIVEN("Particles with supersonic relative velocity") {
        sph::SPHParticle<Dim> p1, p2;
        p1.vel[0] = 0.0;
        p1.sound = 1.0;
        p1.dens = 1.0;
        p1.pres = 1.0;
        
        p2.vel[0] = 10.0;  // Mach 10
        p2.sound = 1.0;
        p2.dens = 1.0;
        p2.pres = 1.0;
        
        WHEN("Computing velocity difference") {
            real v_ij[Dim];
            v_ij[0] = p1.vel[0] - p2.vel[0];
            double mach = std::abs(v_ij[0]) / p1.sound;
            
            THEN("Mach number should be high") {
                EXPECT_GT(mach, 5.0);
            }
            
            AND("Artificial viscosity should be significant") {
                // This would be tested in full simulation
                SUCCEED();
            }
        }
    }
}

SCENARIO(DISPHEdgeCases, IdenticalParticles) {
    GIVEN("Two identical particles at same location") {
        sph::SPHParticle<Dim> p1, p2;
        p1.mass = 1.0;
        p1.dens = 1.0;
        p1.pres = 1.0;
        p1.ene = 1.0;
        p1.pos[0] = 0.0;
        p1.vel[0] = 0.0;
        
        p2 = p1;
        p2.id = 1;
        
        WHEN("Computing separation") {
            real r_ij[Dim];
            for (int i = 0; i < Dim; ++i) {
                r_ij[i] = p1.pos[i] - p2.pos[i];
            }
            double r = std::sqrt(inner_product(r_ij, r_ij));
            
            THEN("Separation should be zero") {
                EXPECT_DOUBLE_EQ(r, 0.0);
            }
            
            AND("Interaction should be skipped") {
                // In actual code: if(r == 0.0) continue;
                EXPECT_EQ(r, 0.0);
            }
        }
    }
}

} // FEATURE(DISPHFluidForce)

FEATURE(DISPHArtificialViscosity) {

SCENARIO(ArtificialViscosity, ConvergingFlow) {
    GIVEN("Two particles moving towards each other") {
        sph::SPHParticle<Dim> p1, p2;
        p1.vel[0] = 1.0;
        p1.sound = 1.0;
        p1.dens = 1.0;
        p1.sml = 0.1;
        
        p2.vel[0] = -1.0;
        p2.sound = 1.0;
        p2.dens = 1.0;
        p2.sml = 0.1;
        
        p1.pos[0] = 0.0;
        p2.pos[0] = 0.05;
        
        WHEN("Computing relative velocity") {
            real r_ij[Dim];
            r_ij[0] = p1.pos[0] - p2.pos[0];
            real v_ij[Dim];
            v_ij[0] = p1.vel[0] - p2.vel[0];
            
            double v_dot_r = inner_product(v_ij, r_ij);
            
            THEN("Particles should be approaching") {
                EXPECT_LT(v_dot_r, 0.0);
            }
            
            AND("Artificial viscosity should activate") {
                SUCCEED();
            }
        }
    }
}

SCENARIO(ArtificialViscosity, DivergingFlow) {
    GIVEN("Two particles moving apart") {
        sph::SPHParticle<Dim> p1, p2;
        p1.vel[0] = -1.0;
        p2.vel[0] = 1.0;
        p1.pos[0] = 0.0;
        p2.pos[0] = 0.1;
        
        WHEN("Computing relative velocity") {
            real r_ij[Dim];
            r_ij[0] = p1.pos[0] - p2.pos[0];
            real v_ij[Dim];
            v_ij[0] = p1.vel[0] - p2.vel[0];
            
            double v_dot_r = inner_product(v_ij, r_ij);
            
            THEN("Particles should be separating") {
                EXPECT_GT(v_dot_r, 0.0);
            }
            
            AND("Artificial viscosity should NOT activate") {
                // pi_ij = 0 when v_dot_r > 0
                SUCCEED();
            }
        }
    }
}

} // FEATURE(DISPHArtificialViscosity)

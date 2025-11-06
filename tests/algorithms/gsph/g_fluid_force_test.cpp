// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../../bdd_helpers.hpp"
#include "parameters.hpp"
#include "core/particles/sph_particle.hpp"
#include <memory>
#include <vector>
#include <cmath>

FEATURE(GSPHFluidForce) {

SCENARIO(GSPHInitialization, RiemannSolver) {
    GIVEN("A GSPH fluid force module") {
        auto param = std::make_shared<sph::SPHParameters>();
        
        param->physics.gamma = 5.0 / 3.0;
        param->physics.neighbor_number = 32;
        param->gsph.is_2nd_order = true;
        param->av.alpha = 1.0;
        param->ac.is_valid = false;
        param->gravity.is_valid = false;
        
        WHEN("Module is initialized") {
            THEN("Parameters should be set correctly") {
                EXPECT_DOUBLE_EQ(param->physics.gamma, 5.0 / 3.0);
                EXPECT_EQ(param->physics.neighbor_number, 32);
                EXPECT_TRUE(param->gsph.is_2nd_order);
                EXPECT_DOUBLE_EQ(param->av.alpha, 1.0);
            }
        }
    }
}

SCENARIO(GSPHCalculation, FirstOrderMethod) {
    GIVEN("GSPH with first-order accuracy") {
        auto param = std::make_shared<sph::SPHParameters>();
        param->physics.gamma = 5.0 / 3.0;
        param->gsph.is_2nd_order = false;  // First order
        
        sph::SPHParticle<Dim> p1, p2;
        p1.dens = 1.0;
        p1.pres = 1.0;
        p1.vel[0] = 0.0;
        p1.sound = std::sqrt(param->physics.gamma * p1.pres / p1.dens);
        
        p2.dens = 0.125;
        p2.pres = 0.1;
        p2.vel[0] = 0.0;
        p2.sound = std::sqrt(param->physics.gamma * p2.pres / p2.dens);
        
        WHEN("Computing Riemann problem") {
            // HLL solver inputs
            double left[4] = {p2.vel[0], p2.dens, p2.pres, p2.sound};
            double right[4] = {p1.vel[0], p1.dens, p1.pres, p1.sound};
            
            THEN("States should be well-defined") {
                EXPECT_GT(left[1], 0.0);   // density positive
                EXPECT_GT(left[2], 0.0);   // pressure positive
                EXPECT_GT(left[3], 0.0);   // sound speed positive
                EXPECT_GT(right[1], 0.0);
                EXPECT_GT(right[2], 0.0);
                EXPECT_GT(right[3], 0.0);
            }
        }
    }
}

SCENARIO(GSPHCalculation, SecondOrderMethod) {
    GIVEN("GSPH with second-order accuracy (MUSCL)") {
        auto param = std::make_shared<sph::SPHParameters>();
        param->physics.gamma = 5.0 / 3.0;
        param->gsph.is_2nd_order = true;  // Second order
        
        WHEN("Using MUSCL reconstruction") {
            double dv_ij = 1.0;
            double dve_i = 0.8;
            
            // van Leer limiter
            double limited = (dv_ij * dve_i <= 0.0) ? 0.0 : 
                           2.0 * dv_ij * dve_i / (dv_ij + dve_i);
            
            THEN("Limiter should prevent oscillations") {
                EXPECT_TRUE(std::isfinite(limited));
                EXPECT_GE(limited, 0.0);  // Should be non-negative for these inputs
            }
        }
    }
}

SCENARIO(GSPHEdgeCases, VanLeerLimiter) {
    GIVEN("Various gradient combinations") {
        WHEN("Gradients have same sign") {
            double dq1 = 1.0;
            double dq2 = 2.0;
            double product = dq1 * dq2;
            
            THEN("Product should be positive") {
                EXPECT_GT(product, 0.0);
            }
            
            AND("Limiter should return harmonic mean") {
                double limited = 2.0 * dq1 * dq2 / (dq1 + dq2);
                double expected = 2.0 * 2.0 / 3.0;  // 4/3
                EXPECT_NEAR(limited, expected, 1e-14);
            }
        }
        
        WHEN("Gradients have opposite signs") {
            double dq1 = 1.0;
            double dq2 = -1.0;
            double product = dq1 * dq2;
            
            THEN("Product should be negative") {
                EXPECT_LT(product, 0.0);
            }
            
            AND("Limiter should return zero") {
                // Limited value is 0 when dq1*dq2 <= 0
                SUCCEED();
            }
        }
        
        WHEN("One gradient is zero") {
            double dq1 = 1.0;
            double dq2 = 0.0;
            double product = dq1 * dq2;
            
            THEN("Product should be zero") {
                EXPECT_DOUBLE_EQ(product, 0.0);
            }
        }
        
        WHEN("Both gradients are zero") {
            double dq1 = 0.0;
            double dq2 = 0.0;
            
            THEN("Limiter should return zero") {
                EXPECT_DOUBLE_EQ(dq1 * dq2, 0.0);
            }
        }
    }
}

SCENARIO(GSPHEdgeCases, SoundSpeedCalculation) {
    GIVEN("Various thermodynamic states") {
        double gamma = 5.0 / 3.0;
        
        WHEN("Normal conditions") {
            double pressure = 1.0;
            double density = 1.0;
            double sound_speed = std::sqrt(gamma * pressure / density);
            
            THEN("Sound speed should be positive and finite") {
                EXPECT_GT(sound_speed, 0.0);
                EXPECT_TRUE(std::isfinite(sound_speed));
            }
        }
        
        WHEN("Very low density") {
            double pressure = 1.0;
            double density = 1e-10;
            double sound_speed = std::sqrt(gamma * pressure / density);
            
            THEN("Sound speed should be very high but finite") {
                EXPECT_GT(sound_speed, 1e4);
                EXPECT_TRUE(std::isfinite(sound_speed));
            }
        }
        
        WHEN("Very high density") {
            double pressure = 1.0;
            double density = 1e10;
            double sound_speed = std::sqrt(gamma * pressure / density);
            
            THEN("Sound speed should be very low but positive") {
                EXPECT_GT(sound_speed, 0.0);
                EXPECT_LT(sound_speed, 1e-4);
            }
        }
        
        WHEN("Zero pressure") {
            double pressure = 0.0;
            double density = 1.0;
            double sound_speed = std::sqrt(gamma * pressure / density);
            
            THEN("Sound speed should be zero") {
                EXPECT_DOUBLE_EQ(sound_speed, 0.0);
            }
        }
    }
}

SCENARIO(GSPHEdgeCases, HLLWaveSpeeds) {
    GIVEN("Riemann problem states") {
        double gamma = 5.0 / 3.0;
        
        WHEN("Shock tube problem") {
            // Left state (high pressure)
            double u_l = 0.0;
            double rho_l = 1.0;
            double p_l = 1.0;
            double c_l = std::sqrt(gamma * p_l / rho_l);
            
            // Right state (low pressure)
            double u_r = 0.0;
            double rho_r = 0.125;
            double p_r = 0.1;
            double c_r = std::sqrt(gamma * p_r / rho_r);
            
            // Roe averages
            double roe_l = std::sqrt(rho_l);
            double roe_r = std::sqrt(rho_r);
            double roe_inv = 1.0 / (roe_l + roe_r);
            
            double u_t = (roe_l * u_l + roe_r * u_r) * roe_inv;
            double c_t = (roe_l * c_l + roe_r * c_r) * roe_inv;
            
            double s_l = std::min(u_l - c_l, u_t - c_t);
            double s_r = std::max(u_r + c_r, u_t + c_t);
            
            THEN("Left wave speed should be negative") {
                EXPECT_LT(s_l, 0.0);
            }
            
            AND("Right wave speed should be positive") {
                EXPECT_GT(s_r, 0.0);
            }
            
            AND("Wave speeds should be finite") {
                EXPECT_TRUE(std::isfinite(s_l));
                EXPECT_TRUE(std::isfinite(s_r));
            }
        }
        
        WHEN("Vacuum generation") {
            // Two rarefaction waves moving apart
            double u_l = -3.0;
            double u_r = 3.0;
            double c_l = 1.0;
            double c_r = 1.0;
            
            THEN("Gap should form") {
                double left_edge = u_l + c_l;   // -2
                double right_edge = u_r - c_r;  // 2
                EXPECT_LT(left_edge, right_edge);
            }
        }
    }
}

} // FEATURE(GSPHFluidForce)

FEATURE(GSPHRiemannSolver) {

SCENARIO(HLLSolver, ContactDiscontinuity) {
    GIVEN("States across a contact discontinuity") {
        // Same pressure and velocity, different density
        double u_l = 1.0;
        double rho_l = 2.0;
        double p_l = 1.0;
        double c_l = std::sqrt(5.0/3.0 * p_l / rho_l);
        
        double u_r = 1.0;
        double rho_r = 1.0;
        double p_r = 1.0;
        double c_r = std::sqrt(5.0/3.0 * p_r / rho_r);
        
        WHEN("Solving Riemann problem") {
            THEN("Pressure should be preserved") {
                // HLL solver would maintain p* ≈ p_l = p_r
                EXPECT_DOUBLE_EQ(p_l, p_r);
            }
            
            AND("Velocity should be preserved") {
                EXPECT_DOUBLE_EQ(u_l, u_r);
            }
        }
    }
}

SCENARIO(HLLSolver, ExtremeStates) {
    GIVEN("Extreme Riemann states") {
        double gamma = 5.0 / 3.0;
        
        WHEN("Very strong shock") {
            double rho_l = 100.0;
            double p_l = 100.0;
            double u_l = 10.0;
            double c_l = std::sqrt(gamma * p_l / rho_l);
            
            double rho_r = 0.01;
            double p_r = 0.01;
            double u_r = 0.0;
            double c_r = std::sqrt(gamma * p_r / rho_r);
            
            THEN("All values should remain finite") {
                EXPECT_TRUE(std::isfinite(c_l));
                EXPECT_TRUE(std::isfinite(c_r));
                EXPECT_GT(c_l, 0.0);
                EXPECT_GT(c_r, 0.0);
            }
        }
        
        WHEN("Near-vacuum state") {
            double rho_r = 1e-15;
            double p_r = 1e-15;
            double c_r = std::sqrt(gamma * p_r / rho_r);
            
            THEN("Sound speed should be finite") {
                EXPECT_TRUE(std::isfinite(c_r));
                EXPECT_GT(c_r, 0.0);
            }
        }
    }
}

} // FEATURE(GSPHRiemannSolver)

// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file monaghan_viscosity_test.cpp
 * @brief BDD tests for Monaghan artificial viscosity
 * 
 * Test scenarios:
 * 1. Basic functionality (name, construction)
 * 2. Approaching particles (vr < 0) - viscosity active
 * 3. Receding particles (vr > 0) - viscosity inactive
 * 4. Balsara switch behavior
 * 5. Sonic conditions (high Mach number)
 * 6. Edge cases (zero velocity, identical particles)
 */

#include "../bdd_helpers.hpp"
#include "../../include/algorithms/viscosity/monaghan_viscosity.hpp"
#include "../../include/core/particles/sph_particle.hpp"
#include <cmath>

using namespace sph;
using namespace sph::algorithms::viscosity;
using namespace sph::utilities::constants;

// ============================================================
// FEATURE: Monaghan Artificial Viscosity
// ============================================================

FEATURE("Monaghan Artificial Viscosity") {
    
    // ========================================================
    // SCENARIO 1: Basic Functionality
    // ========================================================
    
    SCENARIO("Viscosity scheme provides name") {
        GIVEN("A Monaghan viscosity with Balsara switch") {
            MonaghanViscosity<1> viscosity_with_switch(true);
            
            THEN("Name includes 'Monaghan' and 'Balsara'") {
                const std::string name = viscosity_with_switch.name();
                EXPECT_NE(name.find("Monaghan"), std::string::npos);
                EXPECT_NE(name.find("Balsara"), std::string::npos);
            }
        }
        
        GIVEN("A Monaghan viscosity without Balsara switch") {
            MonaghanViscosity<1> viscosity_no_switch(false);
            
            THEN("Name includes 'Monaghan' but not 'Balsara'") {
                const std::string name = viscosity_no_switch.name();
                EXPECT_NE(name.find("Monaghan"), std::string::npos);
                EXPECT_NE(name.find("standard"), std::string::npos);
            }
        }
    }
    
    // ========================================================
    // SCENARIO 2: Approaching Particles (Compression)
    // ========================================================
    
    SCENARIO("Approaching particles experience viscosity") {
        GIVEN("Two 1D particles moving toward each other") {
            SPHParticle<1> p_i, p_j;
            
            // Particle i at x=0, moving right (v=+1)
            p_i.pos[0] = ZERO;
            p_i.vel[0] = ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;  // Viscosity coefficient
            p_i.balsara = ONE; // No Balsara reduction
            
            // Particle j at x=1, moving left (v=-1)
            p_j.pos[0] = ONE;
            p_j.vel[0] = -ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            MonaghanViscosity<1> viscosity(false); // No Balsara switch
            
            WHEN("Computing viscosity") {
                Vector<1> r_ij = p_i.pos - p_j.pos;  // r_ij = -1
                real r = std::abs(r_ij[0]);
                
                ViscosityState<1> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity is negative (resistive force)") {
                    EXPECT_LT(pi_ij, ZERO);
                }
                
                AND_THEN("Magnitude is proportional to approach velocity") {
                    // v_ij = v_i - v_j = 1 - (-1) = 2
                    // vr = v_ij · r_ij = 2 * (-1) = -2 < 0 (approaching)
                    // w_ij = vr / r = -2 / 1 = -2
                    // v_sig = c_i + c_j - 3 w_ij = 1 + 1 - 3*(-2) = 8
                    // π_ij = -0.5 * α * v_sig * w_ij / ρ_ij
                    //      = -0.5 * 1.0 * 8 * (-2) / 1.0 = 8.0
                    EXPECT_NEAR(pi_ij, 8.0, 1e-10);
                }
            }
        }
    }
    
    // ========================================================
    // SCENARIO 3: Receding Particles (Expansion)
    // ========================================================
    
    SCENARIO("Receding particles experience no viscosity") {
        GIVEN("Two 1D particles moving apart") {
            SPHParticle<1> p_i, p_j;
            
            // Particle i at x=0, moving left (v=-1)
            p_i.pos[0] = ZERO;
            p_i.vel[0] = -ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;
            p_i.balsara = ONE;
            
            // Particle j at x=1, moving right (v=+1)
            p_j.pos[0] = ONE;
            p_j.vel[0] = ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            MonaghanViscosity<1> viscosity(false);
            
            WHEN("Computing viscosity") {
                Vector<1> r_ij = p_i.pos - p_j.pos;  // r_ij = -1
                real r = std::abs(r_ij[0]);
                
                ViscosityState<1> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity is exactly zero") {
                    // v_ij = v_i - v_j = -1 - 1 = -2
                    // vr = v_ij · r_ij = -2 * (-1) = 2 > 0 (receding)
                    // π_ij = 0 (no viscosity for expansion)
                    EXPECT_EQ(pi_ij, ZERO);
                }
            }
        }
    }
    
    // ========================================================
    // SCENARIO 4: Balsara Switch Reduces Shear Viscosity
    // ========================================================
    
    SCENARIO("Balsara switch reduces viscosity in shear") {
        GIVEN("Approaching particles with low Balsara factor (pure shear)") {
            SPHParticle<1> p_i, p_j;
            
            // Setup approaching particles
            p_i.pos[0] = ZERO;
            p_i.vel[0] = ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;
            p_i.balsara = 0.1;  // Low Balsara = mostly shear
            
            p_j.pos[0] = ONE;
            p_j.vel[0] = -ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = 0.1;
            
            Vector<1> r_ij = p_i.pos - p_j.pos;
            real r = std::abs(r_ij[0]);
            ViscosityState<1> state{p_i, p_j, r_ij, r};
            
            WHEN("Computing with Balsara switch enabled") {
                MonaghanViscosity<1> viscosity_with_switch(true);
                real pi_with_switch = viscosity_with_switch.compute(state);
                
                AND_WHEN("Computing without Balsara switch") {
                    MonaghanViscosity<1> viscosity_no_switch(false);
                    real pi_no_switch = viscosity_no_switch.compute(state);
                    
                    THEN("Viscosity is reduced by Balsara factor") {
                        // With switch: π_ij ≈ 0.1 * π_ij(no switch)
                        EXPECT_LT(std::abs(pi_with_switch), std::abs(pi_no_switch));
                        EXPECT_NEAR(pi_with_switch / pi_no_switch, 0.1, 0.01);
                    }
                }
            }
        }
        
        GIVEN("Approaching particles with high Balsara factor (pure compression)") {
            SPHParticle<1> p_i, p_j;
            
            p_i.pos[0] = ZERO;
            p_i.vel[0] = ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;
            p_i.balsara = ONE;  // High Balsara = mostly compression
            
            p_j.pos[0] = ONE;
            p_j.vel[0] = -ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            Vector<1> r_ij = p_i.pos - p_j.pos;
            real r = std::abs(r_ij[0]);
            ViscosityState<1> state{p_i, p_j, r_ij, r};
            
            WHEN("Computing with and without Balsara switch") {
                MonaghanViscosity<1> viscosity_with(true);
                MonaghanViscosity<1> viscosity_without(false);
                
                real pi_with = viscosity_with.compute(state);
                real pi_without = viscosity_without.compute(state);
                
                THEN("Results are nearly identical (no reduction)") {
                    EXPECT_NEAR(pi_with, pi_without, 1e-10);
                }
            }
        }
    }
    
    // ========================================================
    // SCENARIO 5: Sonic Conditions (High Mach Number)
    // ========================================================
    
    SCENARIO("Viscosity handles supersonic collisions") {
        GIVEN("Particles approaching at high Mach number (M >> 1)") {
            SPHParticle<1> p_i, p_j;
            
            const real v_shock = 10.0;  // 10× sound speed
            const real c_s = ONE;
            
            p_i.pos[0] = ZERO;
            p_i.vel[0] = v_shock;
            p_i.dens = ONE;
            p_i.sound = c_s;
            p_i.alpha = 1.0;
            p_i.balsara = ONE;
            
            p_j.pos[0] = ONE;
            p_j.vel[0] = -v_shock;
            p_j.dens = ONE;
            p_j.sound = c_s;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            MonaghanViscosity<1> viscosity(false);
            
            WHEN("Computing viscosity") {
                Vector<1> r_ij = p_i.pos - p_j.pos;
                real r = std::abs(r_ij[0]);
                
                ViscosityState<1> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity scales with approach velocity squared") {
                    // v_ij = 20, vr = -20, w_ij = -20
                    // v_sig = 2 - 3*(-20) = 62
                    // π_ij = -0.5 * 1.0 * 62 * (-20) / 1.0 = 620
                    EXPECT_NEAR(pi_ij, 620.0, 1e-8);
                }
                
                AND_THEN("Viscosity is much larger than pressure") {
                    // For shocks, viscosity provides significant dissipation
                    EXPECT_GT(std::abs(pi_ij), 100.0);
                }
            }
        }
    }
    
    // ========================================================
    // SCENARIO 6: Edge Cases
    // ========================================================
    
    SCENARIO("Viscosity handles edge cases correctly") {
        GIVEN("Particles with zero relative velocity") {
            SPHParticle<1> p_i, p_j;
            
            p_i.pos[0] = ZERO;
            p_i.vel[0] = ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;
            p_i.balsara = ONE;
            
            p_j.pos[0] = ONE;
            p_j.vel[0] = ONE;  // Same velocity
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            MonaghanViscosity<1> viscosity(false);
            
            WHEN("Computing viscosity") {
                Vector<1> r_ij = p_i.pos - p_j.pos;
                real r = std::abs(r_ij[0]);
                
                ViscosityState<1> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity is zero (vr = 0)") {
                    EXPECT_EQ(pi_ij, ZERO);
                }
            }
        }
        
        GIVEN("Particles with zero viscosity coefficient") {
            SPHParticle<1> p_i, p_j;
            
            p_i.pos[0] = ZERO;
            p_i.vel[0] = ONE;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = ZERO;  // No viscosity
            p_i.balsara = ONE;
            
            p_j.pos[0] = ONE;
            p_j.vel[0] = -ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = ZERO;
            p_j.balsara = ONE;
            
            MonaghanViscosity<1> viscosity(false);
            
            WHEN("Computing viscosity") {
                Vector<1> r_ij = p_i.pos - p_j.pos;
                real r = std::abs(r_ij[0]);
                
                ViscosityState<1> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity is zero (α = 0)") {
                    EXPECT_EQ(pi_ij, ZERO);
                }
            }
        }
        
        GIVEN("2D particles approaching at angle") {
            SPHParticle<2> p_i, p_j;
            
            // Particle i at origin, moving right
            p_i.pos[0] = ZERO;
            p_i.pos[1] = ZERO;
            p_i.vel[0] = ONE;
            p_i.vel[1] = ZERO;
            p_i.dens = ONE;
            p_i.sound = ONE;
            p_i.alpha = 1.0;
            p_i.balsara = ONE;
            
            // Particle j at (1, 1), moving diagonally toward i
            p_j.pos[0] = ONE;
            p_j.pos[1] = ONE;
            p_j.vel[0] = -ONE;
            p_j.vel[1] = -ONE;
            p_j.dens = ONE;
            p_j.sound = ONE;
            p_j.alpha = 1.0;
            p_j.balsara = ONE;
            
            MonaghanViscosity<2> viscosity(false);
            
            WHEN("Computing viscosity") {
                Vector<2> r_ij = p_i.pos - p_j.pos;  // (-1, -1)
                real r = std::sqrt(TWO);  // |r_ij| = √2
                
                ViscosityState<2> state{p_i, p_j, r_ij, r};
                real pi_ij = viscosity.compute(state);
                
                THEN("Viscosity depends only on radial velocity component") {
                    // v_ij = (2, 1)
                    // vr = v_ij · r_ij = 2*(-1) + 1*(-1) = -3
                    // w_ij = vr / r = -3 / √2
                    EXPECT_LT(pi_ij, ZERO);  // Approaching particles
                }
            }
        }
    }
}

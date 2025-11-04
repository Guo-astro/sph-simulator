/**
 * @file parameter_validation_test.cpp
 * @brief BDD tests for parameter validation against particle configuration
 * 
 * This test suite validates that configuration-dependent parameters (CFL, neighbor_number)
 * are checked against actual particle distributions to prevent simulation blow-up.
 */

#include "core/parameter_validator.hpp"
#include "core/parameter_estimator.hpp"
#include "core/simulation_parameters.hpp"
#include "core/sph_particle.hpp"
#include "../bdd_helpers.hpp"
#include <vector>
#include <cmath>

using namespace sph;

FEATURE(ParameterValidation) {

SCENARIO(CFLValidation, SafeConfiguration) {
    GIVEN("A uniform particle distribution with known spacing") {
        const int n = 100;
        const real dx = 0.01;  // Particle spacing
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0;
            particles[i].pos[0] = i * dx;
            particles[i].sml = 2.0 * dx;  // Smoothing length = 2 * spacing
            particles[i].dens = 1.0;
            particles[i].sound = 1.0;  // Sound speed
            particles[i].vel[0] = 0.1;  // Subsonic velocity
            particles[i].acc[0] = 0.0;
        }
        
        WHEN("CFL coefficients are conservative (0.3, 0.125)") {
            real cfl_sound = 0.3;
            real cfl_force = 0.125;
            
            THEN("Validation should pass") {
                EXPECT_NO_THROW(
                    ParameterValidator::validate_cfl(particles, cfl_sound, cfl_force)
                );
            }
        }
    }
}

SCENARIO(CFLValidation, UnsafeConfigurationSound) {
    GIVEN("High resolution particles with large CFL") {
        const int n = 100;
        const real dx = 0.001;  // Very fine spacing
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0e-3;
            particles[i].pos[0] = i * dx;
            particles[i].sml = 2.0 * dx;  // h = 0.002
            particles[i].dens = 1.0;
            particles[i].sound = 1.0;
            particles[i].vel[0] = 0.5;
            particles[i].acc[0] = 0.0;
        }
        
        WHEN("CFL sound is too large (0.8)") {
            real cfl_sound = 0.8;  // Too aggressive for fine resolution
            real cfl_force = 0.125;
            
            THEN("Validation should throw with descriptive error") {
                EXPECT_THROW(
                    ParameterValidator::validate_cfl(particles, cfl_sound, cfl_force),
                    std::runtime_error
                );
            }
        }
    }
}

SCENARIO(CFLValidation, UnsafeConfigurationForce) {
    GIVEN("Particles with high acceleration") {
        const int n = 100;
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0;
            particles[i].pos[0] = i * 0.01;
            particles[i].sml = 0.02;
            particles[i].dens = 1.0;
            particles[i].sound = 1.0;
            particles[i].vel[0] = 0.0;
            particles[i].acc[0] = 100.0;  // Very high acceleration
        }
        
        WHEN("CFL force is too large (0.5)") {
            real cfl_sound = 0.3;
            real cfl_force = 0.5;  // Too aggressive
            
            THEN("Validation should throw") {
                EXPECT_THROW(
                    ParameterValidator::validate_cfl(particles, cfl_sound, cfl_force),
                    std::runtime_error
                );
            }
        }
    }
}

SCENARIO(NeighborNumberValidation, CorrectConfiguration) {
    GIVEN("Uniform 1D particle distribution") {
        const int n = 100;
        const real dx = 0.01;
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = dx;  // Uniform mass
            particles[i].pos[0] = i * dx;
            particles[i].dens = 1.0;
        }
        
        WHEN("Neighbor number matches resolution (4 for 1D)") {
            int neighbor_number = 4;
            real kernel_support = 2.0;  // Cubic spline support radius
            
            THEN("Validation should pass") {
                EXPECT_NO_THROW(
                    ParameterValidator::validate_neighbor_number(
                        particles, neighbor_number, kernel_support
                    )
                );
            }
        }
    }
}

SCENARIO(NeighborNumberValidation, TooFewNeighbors) {
    GIVEN("Dense particle distribution") {
        const int n = 1000;
        const real dx = 0.001;  // Very fine spacing
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = dx;
            particles[i].pos[0] = i * dx;
            particles[i].dens = 1.0;
        }
        
        WHEN("Neighbor number is too small (2)") {
            int neighbor_number = 2;  // Too few for accurate SPH
            real kernel_support = 2.0;
            
            THEN("Validation should throw warning") {
                EXPECT_THROW(
                    ParameterValidator::validate_neighbor_number(
                        particles, neighbor_number, kernel_support
                    ),
                    std::runtime_error
                );
            }
        }
    }
}

SCENARIO(NeighborNumberValidation, TooManyNeighbors) {
    GIVEN("Sparse particle distribution") {
        const int n = 10;
        const real dx = 0.1;  // Coarse spacing
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].id = i;
            particles[i].mass = dx;
            particles[i].pos[0] = i * dx;
            particles[i].dens = 1.0;
        }
        
        WHEN("Neighbor number is excessive (100)") {
            int neighbor_number = 100;  // More neighbors than particles!
            real kernel_support = 2.0;
            
            THEN("Validation should throw") {
                EXPECT_THROW(
                    ParameterValidator::validate_neighbor_number(
                        particles, neighbor_number, kernel_support
                    ),
                    std::runtime_error
                );
            }
        }
    }
}

} // FEATURE(ParameterValidation)

FEATURE(ParameterEstimation) {

SCENARIO(CFLEstimation, StandardConfiguration) {
    GIVEN("Typical particle configuration") {
        real dx = 0.01;  // Particle spacing
        real sound_speed = 1.0;
        real max_acceleration = 1.0;
        
        WHEN("Estimating safe CFL coefficients") {
            std::pair<real, real> cfl_pair = ParameterEstimator::suggest_cfl(
                dx, sound_speed, max_acceleration
            );
            real cfl_sound = cfl_pair.first;
            real cfl_force = cfl_pair.second;
            
            THEN("CFL values should be conservative") {
                EXPECT_GT(cfl_sound, 0.0);
                EXPECT_LE(cfl_sound, 0.5);  // Should be safe
                EXPECT_GT(cfl_force, 0.0);
                EXPECT_LE(cfl_force, 0.25);
            }
            
            AND("Values should satisfy CFL condition") {
                real dt_sound = cfl_sound * dx / sound_speed;
                real dt_force = cfl_force * std::sqrt(dx / max_acceleration);
                
                EXPECT_GT(dt_sound, 0.0);
                EXPECT_GT(dt_force, 0.0);
            }
        }
    }
}

SCENARIO(CFLEstimation, HighResolution) {
    GIVEN("Very fine particle spacing") {
        real dx = 0.0001;  // Extremely fine
        real sound_speed = 1.0;
        real max_acceleration = 10.0;
        
        WHEN("Estimating CFL") {
            std::pair<real, real> cfl_pair = ParameterEstimator::suggest_cfl(
                dx, sound_speed, max_acceleration
            );
            real cfl_sound = cfl_pair.first;
            real cfl_force = cfl_pair.second;
            
            THEN("CFL should still be reasonable") {
                // Even for fine resolution, CFL shouldn't be too small
                EXPECT_GE(cfl_sound, 0.1);
                EXPECT_GE(cfl_force, 0.05);
            }
        }
    }
}

SCENARIO(NeighborEstimation, OneDimensional) {
    GIVEN("1D particle spacing") {
        real dx = 0.01;
        real kernel_support = 2.0;  // Cubic spline
        int dim = 1;
        
        WHEN("Estimating neighbor number") {
            int neighbor_num = ParameterEstimator::suggest_neighbor_number(
                dx, kernel_support, dim
            );
            
            THEN("Should suggest appropriate count for 1D") {
                // For 1D with h=2*dx, support radius covers ~4 particles
                EXPECT_GE(neighbor_num, 4);
                EXPECT_LE(neighbor_num, 10);  // Not too many
            }
        }
    }
}

SCENARIO(ParameterSuggestion, FromParticles) {
    GIVEN("An actual particle distribution") {
        const int n = 100;
        const real dx = 0.01;
        std::vector<SPHParticle<DIM>> particles(n);
        
        for (int i = 0; i < n; ++i) {
            particles[i].pos[0] = i * dx;
            particles[i].sml = 2.0 * dx;
            particles[i].sound = 1.0;
            particles[i].vel[0] = 0.1;
            particles[i].acc[0] = 0.5;
        }
        
        WHEN("Analyzing particle configuration") {
            auto config = ParameterEstimator::analyze_particle_config(particles);
            
            THEN("Should extract key properties") {
                EXPECT_GT(config.min_spacing, 0.0);
                EXPECT_GT(config.max_sound_speed, 0.0);
                EXPECT_GE(config.max_acceleration, 0.0);
            }
            
            AND("Should suggest appropriate parameters") {
                auto suggestions = ParameterEstimator::suggest_parameters(particles);
                
                EXPECT_GT(suggestions.cfl_sound, 0.0);
                EXPECT_GT(suggestions.cfl_force, 0.0);
                EXPECT_GT(suggestions.neighbor_number, 0);
            }
        }
    }
}

} // FEATURE(ParameterEstimation)

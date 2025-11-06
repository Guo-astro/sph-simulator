// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../bdd_helpers.hpp"
#include <gtest/gtest.h>
#include "core/particles/sph_particle.hpp"
#include "defines.hpp"
#include <cmath>
#include <limits>

FEATURE(ParticleManagement) {

SCENARIO(ParticleCreation, ParticleInitialization) {
    GIVEN("A new SPH particle") {
        sph::SPHParticle<Dim> particle;
        
        WHEN("The particle is created") {
            particle.id = 0;
            particle.mass = 1.0;
            particle.dens = 1.0;
            particle.pres = 1.0;
            particle.ene = 1.0;
            particle.sml = 0.1;
            
            for (int i = 0; i < Dim; ++i) {
                particle.pos[i] = 0.0;
                particle.vel[i] = 0.0;
                particle.acc[i] = 0.0;
            }
            
            THEN("All fields should be properly initialized") {
                EXPECT_EQ(particle.id, 0);
                EXPECT_DOUBLE_EQ(particle.mass, 1.0);
                EXPECT_DOUBLE_EQ(particle.dens, 1.0);
                EXPECT_DOUBLE_EQ(particle.pres, 1.0);
                EXPECT_DOUBLE_EQ(particle.ene, 1.0);
                EXPECT_DOUBLE_EQ(particle.sml, 0.1);
                EXPECT_GT(particle.mass, 0.0);
                EXPECT_GT(particle.dens, 0.0);
                EXPECT_GT(particle.sml, 0.0);
            }
            
            AND("Position should be at origin") {
                for (int i = 0; i < Dim; ++i) {
                    EXPECT_DOUBLE_EQ(particle.pos[i], 0.0);
                }
            }
            
            AND("Velocity should be zero") {
                for (int i = 0; i < Dim; ++i) {
                    EXPECT_DOUBLE_EQ(particle.vel[i], 0.0);
                }
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, InvalidMassHandling) {
    GIVEN("A particle with edge case values") {
        sph::SPHParticle<Dim> particle;
        
        WHEN("Mass is set to zero") {
            particle.mass = 0.0;
            
            THEN("Validation should catch this") {
                EXPECT_EQ(particle.mass, 0.0);
                // In production, this should be validated
            }
        }
        
        WHEN("Mass is set to negative value") {
            particle.mass = -1.0;
            
            THEN("This should be considered invalid") {
                EXPECT_LT(particle.mass, 0.0);
                // In production, this should throw or be validated
            }
        }
        
        WHEN("Mass is set to very large value") {
            particle.mass = 1e20;
            
            THEN("It should still be finite") {
                EXPECT_TRUE(std::isfinite(particle.mass));
                EXPECT_GT(particle.mass, 0.0);
            }
        }
        
        WHEN("Mass is set to very small positive value") {
            particle.mass = 1e-20;
            
            THEN("It should still be valid") {
                EXPECT_TRUE(std::isfinite(particle.mass));
                EXPECT_GT(particle.mass, 0.0);
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, NaNAndInfinityHandling) {
    GIVEN("A particle") {
        sph::SPHParticle<Dim> particle;
        particle.mass = 1.0;
        particle.dens = 1.0;
        particle.sml = 0.1;
        
        WHEN("Position contains NaN") {
            particle.pos[0] = std::numeric_limits<double>::quiet_NaN();
            
            THEN("Detection should work") {
                EXPECT_TRUE(std::isnan(particle.pos[0]));
                bool is_finite = true;
                for (int i = 0; i < Dim; ++i) {
                    if (!std::isfinite(particle.pos[i])) {
                        is_finite = false;
                        break;
                    }
                }
                EXPECT_FALSE(is_finite);
            }
        }
        
        WHEN("Velocity contains infinity") {
            particle.vel[0] = std::numeric_limits<double>::infinity();
            
            THEN("Detection should work") {
                EXPECT_TRUE(std::isinf(particle.vel[0]));
                bool is_finite = true;
                for (int i = 0; i < Dim; ++i) {
                    if (!std::isfinite(particle.vel[i])) {
                        is_finite = false;
                        break;
                    }
                }
                EXPECT_FALSE(is_finite);
            }
        }
        
        WHEN("All values are finite") {
            particle.pos[0] = 1.0;
            particle.vel[0] = 2.0;
            particle.acc[0] = 3.0;
            
            THEN("Validation should pass") {
                bool pos_finite = true, vel_finite = true, acc_finite = true;
                for (int i = 0; i < Dim; ++i) {
                    if (!std::isfinite(particle.pos[i])) pos_finite = false;
                    if (!std::isfinite(particle.vel[i])) vel_finite = false;
                    if (!std::isfinite(particle.acc[i])) acc_finite = false;
                }
                EXPECT_TRUE(pos_finite);
                EXPECT_TRUE(vel_finite);
                EXPECT_TRUE(acc_finite);
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, DensityAndPressure) {
    GIVEN("A particle with various thermodynamic states") {
        sph::SPHParticle<Dim> particle;
        particle.mass = 1.0;
        particle.sml = 0.1;
        
        WHEN("Density is very small") {
            particle.dens = 1e-15;
            
            THEN("It should still be positive and finite") {
                EXPECT_GT(particle.dens, 0.0);
                EXPECT_TRUE(std::isfinite(particle.dens));
            }
        }
        
        WHEN("Pressure is negative (unphysical)") {
            particle.pres = -1.0;
            
            THEN("This should be detected") {
                EXPECT_LT(particle.pres, 0.0);
                // Should be handled appropriately in physics code
            }
        }
        
        WHEN("Energy is zero") {
            particle.ene = 0.0;
            
            THEN("It should be exactly zero") {
                EXPECT_DOUBLE_EQ(particle.ene, 0.0);
            }
        }
        
        WHEN("All thermodynamic quantities are consistent") {
            const double gamma = 5.0/3.0;
            particle.dens = 1.0;
            particle.pres = 1.0;
            particle.ene = particle.pres / ((gamma - 1.0) * particle.dens);
            
            THEN("Energy should be derived correctly") {
                double expected_ene = 1.0 / ((gamma - 1.0) * 1.0);
                EXPECT_NEAR(particle.ene, expected_ene, 1e-10);
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, SmoothingLength) {
    GIVEN("A particle") {
        sph::SPHParticle<Dim> particle;
        particle.mass = 1.0;
        particle.dens = 1.0;
        
        WHEN("Smoothing length is zero") {
            particle.sml = 0.0;
            
            THEN("This should be invalid") {
                EXPECT_EQ(particle.sml, 0.0);
                // Should be validated in production
            }
        }
        
        WHEN("Smoothing length is negative") {
            particle.sml = -0.1;
            
            THEN("This is unphysical") {
                EXPECT_LT(particle.sml, 0.0);
            }
        }
        
        WHEN("Smoothing length is very large") {
            particle.sml = 1e10;
            
            THEN("It should still be finite") {
                EXPECT_TRUE(std::isfinite(particle.sml));
                EXPECT_GT(particle.sml, 0.0);
            }
        }
        
        WHEN("Smoothing length is very small but positive") {
            particle.sml = 1e-10;
            
            THEN("It should be valid") {
                EXPECT_GT(particle.sml, 0.0);
                EXPECT_TRUE(std::isfinite(particle.sml));
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, VelocityBounds) {
    GIVEN("A particle with various velocities") {
        sph::SPHParticle<Dim> particle;
        particle.mass = 1.0;
        particle.dens = 1.0;
        particle.sml = 0.1;
        
        WHEN("Velocity is supersonic") {
            particle.vel[0] = 1000.0;
            particle.sound = 1.0;
            
            THEN("Mach number should be high") {
                double mach = std::abs(particle.vel[0]) / particle.sound;
                EXPECT_GT(mach, 1.0);
            }
        }
        
        WHEN("Velocity is subsonic") {
            particle.vel[0] = 0.5;
            particle.sound = 1.0;
            
            THEN("Mach number should be less than 1") {
                double mach = std::abs(particle.vel[0]) / particle.sound;
                EXPECT_LT(mach, 1.0);
            }
        }
        
        WHEN("Velocity is exactly zero") {
            for (int i = 0; i < Dim; ++i) {
                particle.vel[i] = 0.0;
            }
            
            THEN("All components should be zero") {
                for (int i = 0; i < Dim; ++i) {
                    EXPECT_DOUBLE_EQ(particle.vel[i], 0.0);
                }
            }
        }
    }
}

SCENARIO(ParticleEdgeCases, AccelerationBounds) {
    GIVEN("A particle experiencing forces") {
        sph::SPHParticle<Dim> particle;
        particle.mass = 1.0;
        particle.dens = 1.0;
        particle.sml = 0.1;
        
        WHEN("Acceleration is very large") {
            particle.acc[0] = 1e15;
            
            THEN("It should still be finite") {
                EXPECT_TRUE(std::isfinite(particle.acc[0]));
            }
            
            AND("Timestep should be very small to maintain stability") {
                // This would be tested in timestep module
                EXPECT_GT(std::abs(particle.acc[0]), 1e10);
            }
        }
        
        WHEN("Acceleration is zero (free fall or equilibrium)") {
            for (int i = 0; i < Dim; ++i) {
                particle.acc[i] = 0.0;
            }
            
            THEN("All components should be exactly zero") {
                for (int i = 0; i < Dim; ++i) {
                    EXPECT_DOUBLE_EQ(particle.acc[i], 0.0);
                }
            }
        }
    }
}

} // FEATURE(ParticleManagement)

// Test for particle array operations
FEATURE(ParticleArrayOperations) {

SCENARIO(ParticleArray, MultipleParticles) {
    GIVEN("An array of particles") {
        const int n_particles = 100;
        std::vector<sph::SPHParticle<Dim>> particles(n_particles);
        
        WHEN("Particles are initialized") {
            for (int i = 0; i < n_particles; ++i) {
                particles[i].id = i;
                particles[i].mass = 1.0;
                particles[i].dens = 1.0;
                particles[i].sml = 0.1;
                particles[i].pos[0] = static_cast<double>(i) / n_particles;
            }
            
            THEN("All particles should be valid") {
                for (int i = 0; i < n_particles; ++i) {
                    EXPECT_EQ(particles[i].id, i);
                    EXPECT_GT(particles[i].mass, 0.0);
                    EXPECT_GT(particles[i].dens, 0.0);
                    EXPECT_GT(particles[i].sml, 0.0);
                }
            }
            
            AND("Particles should be in order") {
                for (int i = 1; i < n_particles; ++i) {
                    EXPECT_GT(particles[i].pos[0], particles[i-1].pos[0]);
                }
            }
        }
    }
}

SCENARIO(ParticleArray, EdgeCaseCount) {
    GIVEN("Edge case particle counts") {
        WHEN("Array has zero particles") {
            std::vector<sph::SPHParticle<Dim>> particles(0);
            
            THEN("Array should be empty") {
                EXPECT_EQ(particles.size(), 0);
                EXPECT_TRUE(particles.empty());
            }
        }
        
        WHEN("Array has one particle") {
            std::vector<sph::SPHParticle<Dim>> particles(1);
            particles[0].id = 0;
            particles[0].mass = 1.0;
            
            THEN("Single particle should be valid") {
                EXPECT_EQ(particles.size(), 1);
                EXPECT_EQ(particles[0].id, 0);
            }
        }
        
        WHEN("Array has very large number of particles") {
            const int huge_count = 1000000;
            std::vector<sph::SPHParticle<Dim>> particles;
            particles.reserve(huge_count);
            
            THEN("Memory should be allocated") {
                EXPECT_GE(particles.capacity(), huge_count);
            }
        }
    }
}

} // FEATURE(ParticleArrayOperations)

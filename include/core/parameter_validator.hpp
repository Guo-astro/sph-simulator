/**
 * @file parameter_validator.hpp
 * @brief Validates configuration-dependent parameters against particle distributions
 * 
 * This validator ensures that parameters like CFL coefficients and neighbor_number
 * are appropriate for the actual particle configuration, preventing simulation
 * instability and blow-up.
 * 
 * Parameter Categories:
 * - INDEPENDENT: User-specified constants (gamma, boundaries, etc.)
 * - CONSTRAINED: Configuration-dependent (CFL, neighbor_number) ⚠ VALIDATED HERE
 * - DERIVED: Calculated from particles (smoothing_length, density)
 */

#pragma once

#include "defines.hpp"
#include "core/sph_particle.hpp"
#include "parameters.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

namespace sph {

/**
 * @brief Static validator for configuration-dependent parameters
 * 
 * Usage:
 * @code
 *   std::vector<SPHParticle<DIM>> particles = initialize_particles();
 *   auto params = builder.build();
 *   
 *   // Validate before running simulation
 *   ParameterValidator::validate_all(particles, params);
 * @endcode
 */
class ParameterValidator {
public:
    /**
     * @brief Validate CFL coefficients against particle configuration
     * 
     * Checks that CFL values will produce stable timesteps given the
     * particle spacing, sound speeds, and accelerations.
     * 
     * @tparam Dim Spatial dimension (1, 2, or 3)
     * @param particles Particle distribution
     * @param cfl_sound CFL coefficient for sound-based timestep
     * @param cfl_force CFL coefficient for force-based timestep
     * @throws std::runtime_error if CFL values are unsafe
     */
    template<int Dim>
    static void validate_cfl(
        const std::vector<SPHParticle<Dim>>& particles,
        real cfl_sound,
        real cfl_force
    );
    
    /**
     * @brief Validate neighbor number against particle spacing
     * 
     * Ensures that the specified neighbor number is appropriate for
     * the particle distribution and kernel support radius.
     * 
     * @tparam Dim Spatial dimension (1, 2, or 3)
     * @param particles Particle distribution
     * @param neighbor_number Expected number of neighbors
     * @param kernel_support Kernel support radius (e.g., 2.0 for cubic spline)
     * @throws std::runtime_error if neighbor number is inappropriate
     */
    template<int Dim>
    static void validate_neighbor_number(
        const std::vector<SPHParticle<Dim>>& particles,
        int neighbor_number,
        real kernel_support
    );
    
    /**
     * @brief Validate all configuration-dependent parameters
     * 
     * Comprehensive validation of CFL, neighbor_number, and other
     * constrained parameters against the particle distribution.
     * 
     * @tparam Dim Spatial dimension (1, 2, or 3)
     * @param particles Particle distribution
     * @param params Simulation parameters
     * @throws std::runtime_error if any validation fails
     */
    template<int Dim>
    static void validate_all(
        const std::vector<SPHParticle<Dim>>& particles,
        std::shared_ptr<SPHParameters> params
    );

private:
    /**
     * @brief Calculate minimum particle spacing
     */
    template<int Dim>
    static real calculate_min_spacing(const std::vector<SPHParticle<Dim>>& particles);
    
    /**
     * @brief Calculate maximum sound speed
     */
    template<int Dim>
    static real calculate_max_sound_speed(const std::vector<SPHParticle<Dim>>& particles);
    
    /**
     * @brief Calculate maximum acceleration magnitude
     */
    template<int Dim>
    static real calculate_max_acceleration(const std::vector<SPHParticle<Dim>>& particles);
    
    /**
     * @brief Estimate actual neighbor count for a particle
     */
    template<int Dim>
    static int estimate_actual_neighbors(
        const std::vector<SPHParticle<Dim>>& particles,
        int particle_idx,
        real kernel_support
    );
};

} // namespace sph

#include "core/parameter_validator.tpp"

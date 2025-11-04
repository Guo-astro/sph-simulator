/**
 * @file parameter_estimator.hpp
 * @brief Physics-based estimation of SPH stability parameters
 * 
 * This estimator uses von Neumann stability analysis to suggest CFL coefficients
 * and neighbor numbers that ensure numerical stability. Values are NOT arbitrary -
 * they come from theoretical analysis and decades of SPH literature.
 * 
 * CFL THEORY:
 * -----------
 * The Courant-Friedrichs-Lewy (CFL) condition ensures numerical stability by
 * limiting the timestep based on wave propagation and force timescales:
 * 
 * 1. Sound-based timestep (Monaghan 2005):
 *    dt_sound = CFL_sound * h / (c_s + |v|)
 *    Ensures acoustic waves don't propagate more than one smoothing length
 *    per timestep. Typical safe range: 0.25 - 0.4
 * 
 * 2. Force-based timestep (Monaghan 1989, Morris 1997):
 *    dt_force = CFL_force * sqrt(h / |a_max|)
 *    Ensures accelerations don't cause large particle displacements.
 *    Typical safe range: 0.125 - 0.25
 * 
 * NEIGHBOR NUMBER:
 * ----------------
 * Calculated from kernel support volume to ensure sufficient particles within
 * smoothing radius for accurate density/gradient estimation.
 * 
 * References:
 * - Monaghan, J.J. (1989). "On the problem of penetration in particle methods"
 * - Morris, Monaghan (1997). "A switch to reduce SPH viscosity"
 * - Monaghan, J.J. (2005). "Smoothed particle hydrodynamics"
 */

#pragma once

#include "defines.hpp"
#include "particle.hpp"
#include <vector>
#include <utility>

namespace sph {

/**
 * @brief Configuration analysis results
 */
struct ParticleConfig {
    real min_spacing;          ///< Minimum distance between particles
    real avg_spacing;          ///< Average particle spacing
    real max_sound_speed;      ///< Maximum sound speed in distribution
    real max_velocity;         ///< Maximum velocity magnitude
    real max_acceleration;     ///< Maximum acceleration magnitude
    int dimension;             ///< Effective dimensionality
};

/**
 * @brief Parameter suggestions
 */
struct ParameterSuggestions {
    real cfl_sound;           ///< Suggested CFL for sound timestep
    real cfl_force;           ///< Suggested CFL for force timestep
    int neighbor_number;      ///< Suggested neighbor count
    std::string rationale;    ///< Explanation of suggestions
};

/**
 * @brief Static estimator for safe parameter values
 * 
 * Usage:
 * @code
 *   std::vector<SPHParticle> particles = initialize_particles();
 *   
 *   // Get suggestions
 *   auto suggestions = ParameterEstimator::suggest_parameters(particles);
 *   
 *   // Use in builder
 *   auto params = SimulationParametersBuilder()
 *       .with_cfl(suggestions.cfl_sound, suggestions.cfl_force)
 *       .with_physics(PhysicsParametersBuilder()
 *           .with_neighbor_number(suggestions.neighbor_number)
 *           .with_gamma(1.4)
 *           .build())
 *       // ... rest of parameters
 *       .build();
 * @endcode
 */
class ParameterEstimator {
public:
    /**
     * @brief Suggest CFL coefficients from stability analysis
     * 
     * Uses von Neumann stability theory to calculate safe CFL values:
     * 
     * CFL_sound: Controls dt_sound = CFL_sound * h / (c_s + |v|)
     *   - Baseline: 0.3 (Monaghan 2005)
     *   - Fine resolution: up to 0.35-0.4
     *   - Coarse resolution: down to 0.2
     *   - High Mach number: reduced by 25%
     * 
     * CFL_force: Controls dt_force = CFL_force * sqrt(h / |a|)
     *   - Baseline: 0.25 (Monaghan 1989)
     *   - Strong forces: down to 0.15-0.2
     *   - Weak forces: up to 0.3
     * 
     * These are NOT arbitrary constants - they come from decades of
     * SPH research on numerical stability.
     * 
     * @param particle_spacing Characteristic particle spacing (h)
     * @param sound_speed Maximum sound speed (c_s)
     * @param max_acceleration Maximum acceleration magnitude (|a|)
     * @return Pair of (cfl_sound, cfl_force) based on stability analysis
     */
    static std::pair<real, real> suggest_cfl(
        real particle_spacing,
        real sound_speed,
        real max_acceleration
    );
    
    /**
     * @brief Suggest neighbor number for given resolution
     * 
     * Calculates appropriate neighbor count based on particle spacing,
     * kernel support radius, and dimensionality.
     * 
     * @param particle_spacing Characteristic particle spacing
     * @param kernel_support Kernel support radius (e.g., 2.0 for cubic spline)
     * @param dimension Spatial dimension (1, 2, or 3)
     * @return Suggested neighbor number
     */
    static int suggest_neighbor_number(
        real particle_spacing,
        real kernel_support,
        int dimension
    );
    
    /**
     * @brief Analyze particle configuration
     * 
     * Extracts characteristic properties from particle distribution.
     * 
     * @param particles Particle distribution
     * @return Configuration analysis
     */
    static ParticleConfig analyze_particle_config(
        const std::vector<SPHParticle>& particles
    );
    
    /**
     * @brief Suggest all parameters from particle distribution
     * 
     * Comprehensive analysis and suggestion for all configuration-dependent
     * parameters. This is the recommended entry point.
     * 
     * @param particles Particle distribution
     * @param kernel_support Kernel support radius (default: 2.0)
     * @return Complete parameter suggestions with rationale
     */
    static ParameterSuggestions suggest_parameters(
        const std::vector<SPHParticle>& particles,
        real kernel_support = 2.0
    );

private:
    /**
     * @brief Calculate characteristic spacing in 1D
     */
    static real calculate_spacing_1d(const std::vector<SPHParticle>& particles);
    
    /**
     * @brief Estimate effective dimension from particle distribution
     */
    static int estimate_dimension(const std::vector<SPHParticle>& particles);
    
    /**
     * @brief Generate rationale string explaining suggestions
     */
    static std::string generate_rationale(
        const ParticleConfig& config,
        const ParameterSuggestions& suggestions
    );
};

} // namespace sph

/**
 * @file ghost_particle_coordinator.hpp
 * @brief Coordinator for ghost particle lifecycle management
 * 
 * Orchestrates ghost particle generation and updates in sync with simulation state.
 * Ensures kernel support radius is calculated correctly and ghost manager is called
 * at appropriate times during initialization and time integration.
 * 
 * Key responsibilities:
 * - Calculate kernel support radius from particle smoothing lengths
 * - Initialize ghost particles after smoothing lengths are computed
 * - Update ghost particles during time integration
 * - Provide state queries for ghost system
 * 
 * Design principles:
 * - Single responsibility: ghost lifecycle coordination only
 * - Fail-fast: validates preconditions (non-zero smoothing lengths)
 * - Defensive: handles null ghost_manager gracefully
 * - Transparent: provides logging for diagnostics
 * 
 * @see GhostParticleManager
 * @see Simulation
 */

#pragma once

#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/ghost_particle_manager.hpp"
#include "logger.hpp"
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace sph {

/**
 * @brief Coordinator for managing ghost particle lifecycle
 * 
 * Encapsulates the coordination logic for:
 * - Kernel support radius calculation
 * - Ghost particle generation
 * - Ghost particle updates
 * 
 * Ensures these operations are performed in the correct order and with
 * valid preconditions, reducing coupling in the Solver class.
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class GhostParticleCoordinator {
public:
    /**
     * @brief Construct coordinator for given simulation
     * @param sim Shared pointer to simulation (must not be null)
     * @throws std::invalid_argument if sim is null
     */
    explicit GhostParticleCoordinator(std::shared_ptr<Simulation<Dim>> sim);
    
    /**
     * @brief Initialize ghosts after smoothing lengths are calculated
     * 
     * Call this AFTER pre-interaction has computed smoothing lengths for all
     * real particles. This function:
     * 1. Validates smoothing lengths are positive
     * 2. Calculates kernel support radius from maximum sml
     * 3. Configures ghost_manager with support radius
     * 4. Generates ghost particles
     * 
     * @param real_particles Real particle vector (must have valid sml values)
     * @throws std::logic_error if any particle has sml <= 0
     */
    void initialize_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Update ghosts during time integration
     * 
     * Call this at the beginning of each integration step, before neighbor
     * search. This function:
     * 1. Recalculates kernel support from current smoothing lengths
     * 2. Updates ghost_manager configuration
     * 3. Updates ghost particle positions and properties
     * 
     * @param real_particles Current real particle vector
     * @throws std::logic_error if any particle has sml <= 0
     */
    void update_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Update ghost calculated properties after density/pressure calculation
     * 
     * Call this after m_pre->calculation() to ensure ghost densities, pressures,
     * and energies match their source real particles' updated values.
     * 
     * @param real_particles Real particles with updated calculated properties
     */
    void update_ghost_properties(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Query whether ghost system is active
     * @return true if ghost_manager exists and has generated ghosts
     */
    bool has_ghosts() const;
    
    /**
     * @brief Get current ghost particle count
     * @return Number of ghost particles, or 0 if ghost system disabled
     */
    size_t ghost_count() const;
    
    /**
     * @brief Get current kernel support radius
     * @return Kernel support radius (2.0 * max_sml for cubic spline)
     */
    real get_kernel_support_radius() const;

private:
    /**
     * @brief Calculate kernel support radius from particle smoothing lengths
     * 
     * For cubic spline kernel, support = 2.0 * h
     * This function finds maximum smoothing length and multiplies by support factor.
     * 
     * @param particles Particle vector
     * @return Kernel support radius
     * @throws std::logic_error if any sml <= 0
     */
    real calculate_kernel_support(const std::vector<SPHParticle<Dim>>& particles) const;
    
    /**
     * @brief Validate smoothing lengths are positive
     * @param particles Particle vector to validate
     * @throws std::logic_error if any particle has sml <= 0 with detailed message
     */
    void validate_smoothing_lengths(const std::vector<SPHParticle<Dim>>& particles) const;
    
    /**
     * @brief Log ghost system state for diagnostics
     * @param context Description of calling context (e.g., "initialize_ghosts")
     */
    void log_ghost_state(const std::string& context) const;
    
    /// Shared pointer to simulation (never null after construction)
    std::shared_ptr<Simulation<Dim>> m_sim;
    
    /// Last calculated kernel support radius
    real m_last_kernel_support{0.0};
    
    /// Cubic spline kernel support factor (compact support = 2h)
    static constexpr real CUBIC_SPLINE_SUPPORT_FACTOR = 2.0;
};

} // namespace sph

#include "core/ghost_particle_coordinator.tpp"

/**
 * @file artificial_viscosity.hpp
 * @brief Abstract interface for SPH artificial viscosity schemes
 * 
 * Artificial viscosity in SPH serves several critical purposes:
 * 1. Capture shocks without explicit shock-fitting
 * 2. Prevent particle interpenetration
 * 3. Stabilize the simulation near discontinuities
 * 
 * The viscosity term π_ij appears in the momentum and energy equations:
 *   dv_i/dt = -Σ m_j (P_i/ρ_i² + P_j/ρ_j² + π_ij) ∇W_ij
 *   de_i/dt =  Σ m_j (P_i/ρ_i² + π_ij/2) v_ij · ∇W_ij
 * 
 * References:
 * - Monaghan & Gingold (1983): "Shock simulation by the particle method SPH"
 * - Monaghan (1997): "SPH and Riemann solvers"
 * - Morris & Monaghan (1997): "A switch to reduce SPH viscosity"
 * - Rosswog (2015): "SPH methods in astrophysical applications"
 */

#pragma once

#include "../../core/sph_particle.hpp"
#include "../../core/vector.hpp"
#include "../../defines.hpp"

namespace sph {
namespace algorithms {
namespace viscosity {

/**
 * @brief Viscosity parameters for a particle pair
 * 
 * Contains all information needed to compute viscosity between two particles
 */
template<int Dim>
struct ViscosityState {
    const SPHParticle<Dim>& p_i;   ///< Particle i
    const SPHParticle<Dim>& p_j;   ///< Particle j
    Vector<Dim> r_ij;               ///< Position difference: r_i - r_j
    real r;                         ///< Distance |r_ij|
};

/**
 * @brief Abstract base class for artificial viscosity schemes
 * 
 * Defines the interface for computing artificial viscosity π_ij between
 * particle pairs. Different implementations provide different viscosity
 * models (Monaghan, Rosswog, etc.)
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class ArtificialViscosity {
public:
    virtual ~ArtificialViscosity() = default;
    
    /**
     * @brief Compute artificial viscosity between two particles
     * 
     * @param state Viscosity state containing particle pair information
     * @return Viscosity term π_ij
     * 
     * @note Returns 0 when particles are moving apart (v_ij · r_ij > 0)
     * @note Only applies dissipation for approaching particles
     */
    virtual real compute(const ViscosityState<Dim>& state) const = 0;
    
    /**
     * @brief Get the name of this viscosity scheme
     */
    virtual std::string name() const = 0;
};

} // namespace viscosity
} // namespace algorithms
} // namespace sph

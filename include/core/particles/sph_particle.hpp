/**
 * @file sph_particle.hpp
 * @brief Template-based SPH particle structure
 * 
 * Refactored from vec_t to Vector<Dim> for dimension-agnostic code.
 */

#pragma once

#include "core/utilities/vector.hpp"
#include "../../defines.hpp"

namespace sph {

/**
 * @brief Particle type enumeration
 */
enum class ParticleType : int {
    REAL = 0,   ///< Real/physical particle
    GHOST = 1   ///< Ghost/boundary particle
};

/**
 * @brief SPH Particle data structure
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
struct SPHParticle {
    // Kinematic properties
    Vector<Dim> pos;      ///< Position
    Vector<Dim> vel;      ///< Velocity
    Vector<Dim> vel_p;    ///< Velocity at t + dt/2 (predictor)
    Vector<Dim> acc;      ///< Acceleration
    
    // Thermodynamic properties
    real mass = 0.0;      ///< Particle mass
    real dens = 0.0;      ///< Mass density
    real pres = 0.0;      ///< Pressure
    real ene = 0.0;       ///< Internal energy
    real ene_p = 0.0;     ///< Internal energy at t + dt/2
    real dene = 0.0;      ///< du/dt (energy derivative)
    real sound = 0.0;     ///< Sound speed
    
    // SPH-specific properties
    real sml = 0.0;       ///< Smoothing length
    real gradh = 0.0;     ///< Grad-h term
    
    // Artificial viscosity/conductivity
    real balsara = 0.0;   ///< Balsara switch
    real alpha = 0.0;     ///< AV coefficient
    
    // Optional properties
    real phi = 0.0;       ///< Gravitational potential
    
    // Particle management
    int id = 0;           ///< Particle ID
    int neighbor = 0;     ///< Number of neighbors
    int type = static_cast<int>(ParticleType::REAL);  ///< Particle type: REAL or GHOST
    SPHParticle* next = nullptr;  ///< Linked list pointer
    
    /**
     * @brief Default constructor
     */
    SPHParticle() = default;
    
    /**
     * @brief Get the dimension of this particle
     */
    static constexpr int dimension() { return Dim; }
};

// Type aliases for convenience
using SPHParticle1D = SPHParticle<1>;
using SPHParticle2D = SPHParticle<2>;
using SPHParticle3D = SPHParticle<3>;

} // namespace sph

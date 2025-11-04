#pragma once

#include <type_traits>
#include "vector.hpp"

namespace sph {

/**
 * @brief Dimension policy for 2.5D simulations
 * 
 * In 2.5D simulations:
 * - Hydrodynamics: 2D (r-z plane, azimuthal symmetry)
 * - Gravity: 3D (full 3D space)
 */
struct Dimension2_5D {
    static constexpr int hydro_dim = 2;    // r-z coordinates for hydro
    static constexpr int gravity_dim = 3;  // 3D gravity
    
    // Type aliases for convenience
    using HydroVector = Vector<hydro_dim>;
    using GravityVector = Vector<gravity_dim>;
    
    /**
     * @brief Convert 2D hydro position to 3D gravity position
     * Assumes azimuthal symmetry around z-axis
     */
    static GravityVector hydro_to_gravity(const HydroVector& hydro_pos, real phi = 0.0) {
        return GravityVector{hydro_pos[0] * std::cos(phi), 
                           hydro_pos[0] * std::sin(phi), 
                           hydro_pos[1]};
    }
    
    /**
     * @brief Project 3D gravity position to 2D hydro plane
     * Returns (r, z) coordinates
     */
    static HydroVector gravity_to_hydro(const GravityVector& gravity_pos) {
        real r = std::sqrt(gravity_pos[0]*gravity_pos[0] + gravity_pos[1]*gravity_pos[1]);
        return HydroVector{r, gravity_pos[2]};
    }
    
    /**
     * @brief Convert 2D hydro velocity to 3D gravity velocity
     */
    static GravityVector hydro_velocity_to_gravity(const HydroVector& hydro_vel, real phi = 0.0) {
        // For azimuthal symmetry, v_phi = 0
        return GravityVector{hydro_vel[0] * std::cos(phi), 
                           hydro_vel[0] * std::sin(phi), 
                           hydro_vel[1]};
    }
};

/**
 * @brief Template class for 2.5D SPH simulations
 * 
 * Combines 2D hydrodynamics with 3D gravity calculations
 */
template<typename DimPolicy = Dimension2_5D>
class SPHParticle2_5D {
public:
    using HydroVector = typename DimPolicy::HydroVector;
    using GravityVector = typename DimPolicy::GravityVector;
    
    // Hydrodynamic properties (2D)
    HydroVector pos;      // position in r-z plane
    HydroVector vel;      // velocity in r-z plane
    HydroVector acc;      // acceleration in r-z plane
    
    // Thermodynamic properties (scalar)
    real mass;
    real density;
    real pressure;
    real energy;
    real sml;            // smoothing length
    
    // Gravity properties (3D)
    GravityVector g_pos;  // position for gravity calculations
    GravityVector g_vel;  // velocity for gravity calculations
    GravityVector g_acc;  // gravitational acceleration (3D)
    
    real phi;            // gravitational potential
    int id;
    
    // Linked list for neighbor search
    SPHParticle2_5D* next;
    
    SPHParticle2_5D() : next(nullptr), phi(0.0) {}
    
    /**
     * @brief Update 3D gravity position from 2D hydro position
     * @param azimuthal_angle azimuthal angle for this particle
     */
    void update_gravity_position(real azimuthal_angle = 0.0) {
        g_pos = DimPolicy::hydro_to_gravity(pos, azimuthal_angle);
        g_vel = DimPolicy::hydro_velocity_to_gravity(vel, azimuthal_angle);
    }
    
    /**
     * @brief Get cylindrical radius
     */
    real r() const { return pos[0]; }
    
    /**
     * @brief Get z coordinate
     */
    real z() const { return pos[1]; }
};

// Type alias for standard 2.5D particle
using SPHParticle25D = SPHParticle2_5D<>;

} // namespace sph</content>
<parameter name="filePath">/Users/guo/sph-simulation/include/core/sph_particle_2_5d.hpp
#pragma once

/**
 * @file physics_parameters.hpp
 * @brief Physics-related parameters for SPH simulation
 * 
 * This file contains ONLY parameters that affect the physical behavior:
 * - Equation of state (gamma)
 * - Neighbor search radius (neighbor_number)
 * - Artificial viscosity (dissipation model)
 * - Artificial conductivity (thermal dissipation)
 * - Periodic boundary conditions (physical domain)
 * - Gravity (external forces)
 * 
 * These parameters determine WHAT physics is being simulated.
 */

#include "defines.hpp"
#include <memory>

namespace sph
{

/**
 * @brief Physical parameters for SPH simulation
 * 
 * These parameters define the physical model being simulated.
 */
struct PhysicsParameters {
    
    // Equation of state
    real gamma;              ///< Adiabatic index (must be > 1.0)
    int neighbor_number;     ///< Expected number of neighbors (affects smoothing length)
    
    // Dissipation models
    struct ArtificialViscosity {
        real alpha;                  ///< Viscosity coefficient
        bool use_balsara_switch;     ///< Use Balsara switch to reduce shear viscosity
        bool use_time_dependent;     ///< Time-dependent alpha
        real alpha_max;              ///< Maximum alpha (if time-dependent)
        real alpha_min;              ///< Minimum alpha (if time-dependent)
        real epsilon;                ///< tau = h / (epsilon * c)
    } av;
    
    struct ArtificialConductivity {
        bool is_valid;       ///< Whether to use artificial conductivity
        real alpha;          ///< Conductivity coefficient
    } ac;
    
    // Boundary conditions
    struct Periodic {
        bool is_valid;           ///< Whether periodic boundaries are active
        real range_min[DIM];     ///< Minimum coordinates
        real range_max[DIM];     ///< Maximum coordinates
    } periodic;
    
    // External forces
    struct Gravity {
        bool is_valid;       ///< Whether gravity is active
        real constant;       ///< Gravitational constant G
        real theta;          ///< Tree opening angle for gravity calculation
    } gravity;
};

/**
 * @brief Builder for physics parameters with validation
 */
class PhysicsParametersBuilder {
public:
    PhysicsParametersBuilder();
    
    // Required parameters
    PhysicsParametersBuilder& with_gamma(real gamma);
    PhysicsParametersBuilder& with_neighbor_number(int neighbor_number);
    
    // Optional: Dissipation models
    PhysicsParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    );
    
    PhysicsParametersBuilder& with_artificial_conductivity(real alpha);
    
    // Optional: Boundary conditions
    PhysicsParametersBuilder& with_periodic_boundary(
        const real range_min[DIM],
        const real range_max[DIM]
    );
    
    // Optional: External forces
    PhysicsParametersBuilder& with_gravity(real constant, real theta = 0.5);
    
    // Load from configuration
    PhysicsParametersBuilder& from_json(const char* filename);
    PhysicsParametersBuilder& from_existing(std::shared_ptr<PhysicsParameters> existing);
    
    // Build with validation
    std::shared_ptr<PhysicsParameters> build();
    
private:
    std::shared_ptr<PhysicsParameters> params;
    
    // Required parameter flags
    bool has_gamma = false;
    bool has_neighbor_number = false;
    
    void validate() const;
    bool is_complete() const;
    std::string get_missing_parameters() const;
};

}

#pragma once

#include <memory>
#include <vector>
#include <optional>
#include "core/particles/sph_particle.hpp"
#include "parameters.hpp"
#include "core/boundaries/boundary_types.hpp"
// #include "core/parameters/output_parameters.hpp"  // TODO: Not implemented yet

namespace sph {

/**
 * @brief Initial condition data (pure business logic)
 * 
 * This is a PLAIN DATA OBJECT with no system dependencies.
 * Plugins construct and return this. The framework handles all system initialization.
 * 
 * Benefits:
 * - Pure data (no side effects)
 * - Easy to test (just check values)
 * - No coupling to Simulation internals
 * - Framework can optimize initialization
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
struct InitialCondition {
    /// Particle initial state (positions, velocities, densities, etc.)
    std::vector<SPHParticle<Dim>> particles;
    
    /// SPH algorithm parameters (CFL, kernel, neighbor count, etc.)
    std::shared_ptr<SPHParameters> parameters;
    
    /// Boundary configuration (periodic, mirror, none)
    std::optional<BoundaryConfiguration<Dim>> boundary_config;
    
    // /// Output configuration (formats, unit conversion, etc.)
    // std::optional<OutputConfiguration> output_config;  // TODO: Not implemented yet
    
    // ===== Convenience Builders (Fluent API) =====
    
    /**
     * @brief Start building with particles
     */
    static InitialCondition<Dim> with_particles(std::vector<SPHParticle<Dim>> p) {
        InitialCondition<Dim> ic;
        ic.particles = std::move(p);
        return ic;
    }
    
    /**
     * @brief Set SPH parameters
     * Returns by reference for chaining
     */
    InitialCondition& with_parameters(std::shared_ptr<SPHParameters> params) & {
        parameters = std::move(params);
        return *this;
    }
    
    /**
     * @brief Set SPH parameters (rvalue overload)
     * Returns by value to enable RVO when used in return statement
     */
    InitialCondition&& with_parameters(std::shared_ptr<SPHParameters> params) && {
        parameters = std::move(params);
        return std::move(*this);
    }
    
    /**
     * @brief Set boundary configuration
     * Returns by reference for chaining
     */
    InitialCondition& with_boundaries(BoundaryConfiguration<Dim> config) & {
        boundary_config = std::move(config);
        return *this;
    }
    
    /**
     * @brief Set boundary configuration (rvalue overload)
     * Returns by value to enable RVO when used in return statement
     */
    InitialCondition&& with_boundaries(BoundaryConfiguration<Dim> config) && {
        boundary_config = std::move(config);
        return std::move(*this);
    }
    
    // /**
    //  * @brief Set output configuration (TODO: Not implemented yet)
    //  */
    // InitialCondition& with_output(OutputConfiguration config) {
    //     output_config = std::move(config);
    //     return *this;
    // }
    
    // ===== Validation =====
    
    /**
     * @brief Check if initial condition is valid
     */
    bool is_valid() const {
        return !particles.empty() && parameters != nullptr;
    }
    
    /**
     * @brief Get number of particles
     */
    int particle_count() const {
        return static_cast<int>(particles.size());
    }
};

// Type aliases
template<int Dim>
using IC = InitialCondition<Dim>;

using IC1D = InitialCondition<1>;
using IC2D = InitialCondition<2>;
using IC3D = InitialCondition<3>;

} // namespace sph

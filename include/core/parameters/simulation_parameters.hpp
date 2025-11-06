#pragma once

/**
 * @file simulation_parameters.hpp
 * @brief Top-level simulation orchestration parameters
 * 
 * This file contains high-level simulation control:
 * - Time range (start, end)
 * - SPH algorithm type (SSPH, DISPH, GSPH)
 * - CFL conditions (stability)
 * - Composition of physics, computational, and output parameters
 * 
 * This is the main entry point for configuring a complete simulation.
 */

#include "../../defines.hpp"
#include "core/particles/sph_types.hpp"
#include "core/parameters/physics_parameters.hpp"
#include "core/parameters/computational_parameters.hpp"
#include "core/parameters/output_parameters.hpp"
#include <memory>
#include <string>

namespace sph
{

/**
 * @brief Complete simulation parameters
 * 
 * Composes all parameter categories into a complete simulation configuration.
 */
struct SimulationParameters {
    
    // Time control
    struct Time {
        real start;  ///< Simulation start time
        real end;    ///< Simulation end time
    } time;
    
    // SPH algorithm selection
    SPHType sph_type;
    
    // Stability conditions
    struct CFL {
        real sound;  ///< CFL condition for sound speed
        real force;  ///< CFL condition for forces
    } cfl;
    
    // Category parameters (composition)
    std::shared_ptr<PhysicsParameters> physics;
    std::shared_ptr<ComputationalParameters> computational;
    std::shared_ptr<OutputParameters> output;
};

/**
 * @brief Builder for complete simulation parameters
 * 
 * This builder composes the three parameter categories (physics, computational, output)
 * along with top-level simulation settings.
 */
class SimulationParametersBuilder {
public:
    SimulationParametersBuilder();
    
    // Required: time range
    SimulationParametersBuilder& with_time(real start, real end);
    
    // Required: SPH algorithm type
    SimulationParametersBuilder& with_sph_type(const std::string& type_name);
    
    // Required: CFL conditions
    SimulationParametersBuilder& with_cfl(real sound, real force);
    
    // Required: parameter categories
    SimulationParametersBuilder& with_physics(std::shared_ptr<PhysicsParameters> physics);
    SimulationParametersBuilder& with_computational(std::shared_ptr<ComputationalParameters> computational);
    SimulationParametersBuilder& with_output(std::shared_ptr<OutputParameters> output);
    
    // Convenience: build categories inline
    template<typename PhysicsBuilder>
    SimulationParametersBuilder& with_physics_from(PhysicsBuilder&& builder) {
        return with_physics(builder.build());
    }
    
    template<typename ComputationalBuilder>
    SimulationParametersBuilder& with_computational_from(ComputationalBuilder&& builder) {
        return with_computational(builder.build());
    }
    
    template<typename OutputBuilder>
    SimulationParametersBuilder& with_output_from(OutputBuilder&& builder) {
        return with_output(builder.build());
    }
    
    // Load from configuration (loads all categories)
    SimulationParametersBuilder& from_json_file(const char* filename);
    
    // Build with validation
    std::shared_ptr<SimulationParameters> build();
    
private:
    std::shared_ptr<SimulationParameters> params;
    
    bool has_time = false;
    bool has_sph_type = false;
    bool has_cfl = false;
    bool has_physics = false;
    bool has_computational = false;
    bool has_output = false;
    
    void validate() const;
    bool is_complete() const;
    std::string get_missing_parameters() const;
};

}

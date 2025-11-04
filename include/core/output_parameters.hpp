#pragma once

/**
 * @file output_parameters.hpp
 * @brief Output-related parameters for SPH simulation
 * 
 * This file contains ONLY parameters that control simulation output:
 * - Output directory
 * - Particle data output interval
 * - Energy statistics output interval
 * - Output format options
 * 
 * These parameters determine WHAT and WHEN to write results.
 */

#include "defines.hpp"
#include <memory>
#include <string>

namespace sph
{

/**
 * @brief Output control parameters
 * 
 * These parameters control when and what simulation data is written.
 */
struct OutputParameters {
    
    std::string directory;        ///< Output directory path
    real particle_interval;       ///< Time interval for particle data output
    real energy_interval;         ///< Time interval for energy statistics output
    
    // Future extensions:
    // bool write_velocities;
    // bool write_accelerations;
    // std::string format;  // "binary", "ascii", "hdf5"
};

/**
 * @brief Builder for output parameters
 */
class OutputParametersBuilder {
public:
    OutputParametersBuilder();
    
    // Required: output directory
    OutputParametersBuilder& with_directory(const std::string& directory);
    
    // Required: particle output interval
    OutputParametersBuilder& with_particle_output_interval(real interval);
    
    // Optional: energy output interval (defaults to particle interval)
    OutputParametersBuilder& with_energy_output_interval(real interval);
    
    // Load from configuration
    OutputParametersBuilder& from_json(const char* filename);
    OutputParametersBuilder& from_existing(std::shared_ptr<OutputParameters> existing);
    
    // Build with validation
    std::shared_ptr<OutputParameters> build();
    
private:
    std::shared_ptr<OutputParameters> params;
    
    bool has_directory = false;
    bool has_particle_interval = false;
    
    void validate() const;
    bool is_complete() const;
    std::string get_missing_parameters() const;
};

}

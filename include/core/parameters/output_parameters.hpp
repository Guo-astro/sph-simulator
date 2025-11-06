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
 * - Unit system selection
 * - Metadata generation
 * 
 * These parameters determine WHAT and WHEN to write results.
 */

#include "../../defines.hpp"
#include "../output/units/unit_system.hpp"
#include "../output/writers/output_writer.hpp"
#include <memory>
#include <string>
#include <vector>

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
    
    // New fields for redesigned output system
    std::vector<OutputFormat> formats;  ///< Output formats (CSV, Protobuf)
    UnitSystemType unit_system;         ///< Unit system for output
    bool write_metadata;                ///< Whether to write metadata.json
    
    /**
     * @brief Constructor with defaults
     */
    OutputParameters()
        : directory("output")
        , particle_interval(0.1)
        , energy_interval(0.01)
        , formats({OutputFormat::CSV})  // Default to CSV only
        , unit_system(UnitSystemType::GALACTIC)  // Default to galactic units
        , write_metadata(true)  // Always write metadata by default
    {}
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
    
    // New methods for output system redesign
    
    /**
     * @brief Add an output format
     * @param format Format to add (CSV or PROTOBUF)
     */
    OutputParametersBuilder& with_format(OutputFormat format);
    
    /**
     * @brief Set output formats (replaces any existing)
     * @param formats Vector of output formats
     */
    OutputParametersBuilder& with_formats(const std::vector<OutputFormat>& formats);
    
    /**
     * @brief Set unit system
     * @param unit_system Unit system type
     */
    OutputParametersBuilder& with_unit_system(UnitSystemType unit_system);
    
    /**
     * @brief Set whether to write metadata
     * @param write_metadata True to write metadata.json
     */
    OutputParametersBuilder& with_metadata(bool write_metadata);
    
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

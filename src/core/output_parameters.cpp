#include "core/output_parameters.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdexcept>

namespace sph
{

OutputParametersBuilder::OutputParametersBuilder() 
    : params(std::make_shared<OutputParameters>()) {
    
    // Set defaults
    params->directory = "output";
    params->particle_interval = 0.01;
    params->energy_interval = 0.01;
}

OutputParametersBuilder& OutputParametersBuilder::with_directory(const std::string& directory) {
    params->directory = directory;
    has_directory = true;
    return *this;
}

OutputParametersBuilder& OutputParametersBuilder::with_particle_output_interval(real interval) {
    params->particle_interval = interval;
    has_particle_interval = true;
    return *this;
}

OutputParametersBuilder& OutputParametersBuilder::with_energy_output_interval(real interval) {
    params->energy_interval = interval;
    return *this;
}

OutputParametersBuilder& OutputParametersBuilder::from_json(const char* filename) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    pt::read_json(filename, input);
    
    // Output directory
    if (input.count("outputDirectory")) {
        with_directory(input.get<std::string>("outputDirectory"));
    }
    
    // Particle output interval
    if (input.count("outputTime")) {
        with_particle_output_interval(input.get<real>("outputTime"));
    }
    
    // Energy output interval (defaults to particle interval if not specified)
    if (input.count("energyTime")) {
        with_energy_output_interval(input.get<real>("energyTime"));
    } else if (has_particle_interval) {
        // Default energy interval to particle interval
        params->energy_interval = params->particle_interval;
    }
    
    return *this;
}

OutputParametersBuilder& OutputParametersBuilder::from_existing(
    std::shared_ptr<OutputParameters> existing
) {
    if (!existing) {
        throw std::runtime_error("Cannot load from null OutputParameters");
    }
    
    *params = *existing;
    
    has_directory = true;
    has_particle_interval = true;
    
    return *this;
}

void OutputParametersBuilder::validate() const {
    // Validate particle interval
    if (params->particle_interval <= 0) {
        throw std::runtime_error(
            "Particle output interval must be positive, got " + 
            std::to_string(params->particle_interval)
        );
    }
    
    // Validate energy interval
    if (params->energy_interval <= 0) {
        throw std::runtime_error(
            "Energy output interval must be positive, got " + 
            std::to_string(params->energy_interval)
        );
    }
    
    // Directory can be any valid string, no specific validation needed
}

bool OutputParametersBuilder::is_complete() const {
    return has_directory && has_particle_interval;
}

std::string OutputParametersBuilder::get_missing_parameters() const {
    std::string missing = "Missing required output parameters: ";
    bool first = true;
    
    if (!has_directory) {
        if (!first) missing += ", ";
        missing += "directory";
        first = false;
    }
    if (!has_particle_interval) {
        if (!first) missing += ", ";
        missing += "particle_interval";
        first = false;
    }
    
    return missing;
}

std::shared_ptr<OutputParameters> OutputParametersBuilder::build() {
    if (!is_complete()) {
        throw std::runtime_error(get_missing_parameters());
    }
    
    // If energy interval not explicitly set, default to particle interval
    if (params->energy_interval <= 0 || params->energy_interval == 0.01) {
        params->energy_interval = params->particle_interval;
    }
    
    validate();
    return params;
}

}

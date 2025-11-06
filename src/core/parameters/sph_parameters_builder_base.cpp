#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include "core/parameters/disph_parameters_builder.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include "core/particles/sph_types.hpp"
#include "exception.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <cmath>

namespace sph {

// ==================== SPHParametersBuilderBase Implementation ====================

SPHParametersBuilderBase::SPHParametersBuilderBase() 
    : params(std::make_shared<SPHParameters>()) {
    // Set sensible defaults for optional parameters
    params->time.start = 0.0;
    params->time.energy = 0.01;
    
    params->tree.max_level = 20;
    params->tree.leaf_particle_num = 1;
    
    params->iterative_sml = true;
    
    params->periodic.is_valid = false;
    for (std::size_t i = 0; i < 3; ++i) {
        params->periodic.range_min[i] = 0.0;
        params->periodic.range_max[i] = 1.0;
    }
    
    params->gravity.is_valid = false;
    params->gravity.constant = 1.0;
    params->gravity.theta = 0.5;
}

void SPHParametersBuilderBase::validate_time() const {
    if (!has_time) {
        throw std::runtime_error("Time parameters not set. Call with_time() before building.");
    }
    if (params->time.end <= params->time.start) {
        throw std::runtime_error("End time must be greater than start time.");
    }
    if (params->time.output <= 0.0) {
        throw std::runtime_error("Output interval must be positive.");
    }
}

void SPHParametersBuilderBase::validate_cfl() const {
    if (!has_cfl) {
        throw std::runtime_error("CFL parameters not set. Call with_cfl() before building.");
    }
    if (params->cfl.sound <= 0.0 || params->cfl.sound > 1.0) {
        throw std::runtime_error("CFL sound must be in (0, 1].");
    }
    if (params->cfl.force <= 0.0 || params->cfl.force > 1.0) {
        throw std::runtime_error("CFL force must be in (0, 1].");
    }
}

void SPHParametersBuilderBase::validate_physics() const {
    if (!has_physics) {
        throw std::runtime_error("Physics parameters not set. Call with_physics() before building.");
    }
    if (params->physics.neighbor_number <= 0) {
        throw std::runtime_error("Neighbor number must be positive.");
    }
    if (params->physics.gamma <= 0.0) {
        throw std::runtime_error("Gamma must be positive.");
    }
}

void SPHParametersBuilderBase::validate_kernel() const {
    if (!has_kernel) {
        throw std::runtime_error("Kernel not set. Call with_kernel() before building.");
    }
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_time(real start, real end, real output, real energy) {
    params->time.start = start;
    params->time.end = end;
    params->time.output = output;
    params->time.energy = (energy < 0) ? output : energy;
    has_time = true;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_cfl(real sound, real force) {
    params->cfl.sound = sound;
    params->cfl.force = force;
    has_cfl = true;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_physics(int neighbor_number, real gamma) {
    params->physics.neighbor_number = neighbor_number;
    params->physics.gamma = gamma;
    has_physics = true;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_kernel(const std::string& kernel_name) {
    if (kernel_name == "cubic_spline") {
        params->kernel = KernelType::CUBIC_SPLINE;
    } else if (kernel_name == "wendland" || kernel_name == "wendland_c2") {
        params->kernel = KernelType::WENDLAND;
    } else {
        throw std::runtime_error("Unknown kernel type: " + kernel_name);
    }
    has_kernel = true;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_gravity(real constant, real theta) {
    params->gravity.is_valid = true;
    params->gravity.constant = constant;
    params->gravity.theta = theta;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_tree_params(int max_level, int leaf_particle_num) {
    params->tree.max_level = max_level;
    params->tree.leaf_particle_num = leaf_particle_num;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_periodic_boundary(
    const std::array<real, 3>& range_min,
    const std::array<real, 3>& range_max
) {
    params->periodic.is_valid = true;
    params->periodic.range_min = range_min;
    params->periodic.range_max = range_max;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::with_iterative_smoothing_length(bool enable) {
    params->iterative_sml = enable;
    return *this;
}

SPHParametersBuilderBase& SPHParametersBuilderBase::from_json_file(const char* filename) {
    // Implementation delegated to existing JSON parsing logic
    // This is a placeholder - actual implementation would parse JSON
    throw std::runtime_error("JSON loading not yet implemented in new builder");
}

SPHParametersBuilderBase& SPHParametersBuilderBase::from_json_string(const char* json_content) {
    // Implementation delegated to existing JSON parsing logic
    throw std::runtime_error("JSON loading not yet implemented in new builder");
}

SPHParametersBuilderBase& SPHParametersBuilderBase::from_existing(std::shared_ptr<SPHParameters> existing) {
    if (existing) {
        params = std::make_shared<SPHParameters>(*existing);
        // Mark all as set if they have valid values
        has_time = true;
        has_cfl = true;
        has_physics = true;
        has_kernel = true;
    }
    return *this;
}

AlgorithmParametersBuilder<SSPHTag> SPHParametersBuilderBase::as_ssph() {
    // Validate common parameters before transitioning
    validate_time();
    validate_cfl();
    validate_physics();
    validate_kernel();
    
    params->type = SPHType::SSPH;
    return AlgorithmParametersBuilder<SSPHTag>(params);
}

AlgorithmParametersBuilder<DISPHTag> SPHParametersBuilderBase::as_disph() {
    validate_time();
    validate_cfl();
    validate_physics();
    validate_kernel();
    
    params->type = SPHType::DISPH;
    return AlgorithmParametersBuilder<DISPHTag>(params);
}

AlgorithmParametersBuilder<GSPHTag> SPHParametersBuilderBase::as_gsph() {
    validate_time();
    validate_cfl();
    validate_physics();
    validate_kernel();
    
    params->type = SPHType::GSPH;
    return AlgorithmParametersBuilder<GSPHTag>(params);
}

bool SPHParametersBuilderBase::is_complete() const {
    return has_time && has_cfl && has_physics && has_kernel;
}

std::string SPHParametersBuilderBase::get_missing_parameters() const {
    std::ostringstream oss;
    oss << "Missing required parameters: ";
    
    bool first = true;
    if (!has_time) {
        if (!first) oss << ", ";
        oss << "time";
        first = false;
    }
    if (!has_cfl) {
        if (!first) oss << ", ";
        oss << "cfl";
        first = false;
    }
    if (!has_physics) {
        if (!first) oss << ", ";
        oss << "physics";
        first = false;
    }
    if (!has_kernel) {
        if (!first) oss << ", ";
        oss << "kernel";
        first = false;
    }
    
    return oss.str();
}

} // namespace sph

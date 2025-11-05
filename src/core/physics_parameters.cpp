#include "core/physics_parameters.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <stdexcept>

namespace sph
{

PhysicsParametersBuilder::PhysicsParametersBuilder() 
    : params(std::make_shared<PhysicsParameters>()) {
    
    // Set sensible defaults for optional parameters
    
    // Artificial viscosity defaults
    params->av.alpha = 1.0;
    params->av.use_balsara_switch = true;
    params->av.use_time_dependent = false;
    params->av.alpha_max = 2.0;
    params->av.alpha_min = 0.1;
    params->av.epsilon = 0.2;
    
    // Artificial conductivity defaults (disabled)
    params->ac.is_valid = false;
    params->ac.alpha = 1.0;
    
    // Periodic boundary defaults (disabled)
    params->periodic.is_valid = false;
    for (int i = 0; i < 3; ++i) {
        params->periodic.range_min[i] = 0.0;
        params->periodic.range_max[i] = 1.0;
    }
    
    // Gravity defaults (disabled)
    params->gravity.is_valid = false;
    params->gravity.constant = 1.0;
    params->gravity.theta = 0.5;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_gamma(real gamma) {
    params->gamma = gamma;
    has_gamma = true;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_neighbor_number(int neighbor_number) {
    params->neighbor_number = neighbor_number;
    has_neighbor_number = true;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_artificial_viscosity(
    real alpha,
    bool use_balsara_switch,
    bool use_time_dependent,
    real alpha_max,
    real alpha_min,
    real epsilon
) {
    params->av.alpha = alpha;
    params->av.use_balsara_switch = use_balsara_switch;
    params->av.use_time_dependent = use_time_dependent;
    params->av.alpha_max = alpha_max;
    params->av.alpha_min = alpha_min;
    params->av.epsilon = epsilon;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_artificial_conductivity(real alpha) {
    params->ac.is_valid = true;
    params->ac.alpha = alpha;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_periodic_boundary(
    const std::array<real, 3>& range_min,
    const std::array<real, 3>& range_max
) {
    params->periodic.is_valid = true;
    params->periodic.range_min = range_min;
    params->periodic.range_max = range_max;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::with_gravity(real constant, real theta) {
    params->gravity.is_valid = true;
    params->gravity.constant = constant;
    params->gravity.theta = theta;
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::from_json(const char* filename) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    pt::read_json(filename, input);
    
    // Gamma
    if (input.count("gamma")) {
        with_gamma(input.get<real>("gamma"));
    }
    
    // Neighbor number
    if (input.count("neighborNumber")) {
        with_neighbor_number(input.get<int>("neighborNumber"));
    }
    
    // Artificial viscosity
    if (input.count("avAlpha")) {
        with_artificial_viscosity(
            input.get<real>("avAlpha", 1.0),
            input.get<bool>("useBalsaraSwitch", true),
            input.get<bool>("useTimeDependentAV", false),
            input.get<real>("alphaMax", 2.0),
            input.get<real>("alphaMin", 0.1),
            input.get<real>("epsilonAV", 0.2)
        );
    }
    
    // Artificial conductivity
    if (input.get<bool>("useArtificialConductivity", false)) {
        with_artificial_conductivity(input.get<real>("alphaAC", 1.0));
    }
    
    // Periodic boundary
    if (input.get<bool>("periodic", false)) {
        std::array<real, 3> range_min = {0, 0, 0};
        std::array<real, 3> range_max = {0, 0, 0};
        
        auto& range_min_node = input.get_child("rangeMin");
        auto& range_max_node = input.get_child("rangeMax");
        
        int i = 0;
        for (auto& v : range_min_node) {
            if (i < 3) range_min[i++] = std::stod(v.second.data());
        }
        i = 0;
        for (auto& v : range_max_node) {
            if (i < 3) range_max[i++] = std::stod(v.second.data());
        }
        
        with_periodic_boundary(range_min, range_max);
    }
    
    // Gravity
    if (input.get<bool>("useGravity", false)) {
        with_gravity(
            input.get<real>("G", 1.0),
            input.get<real>("theta", 0.5)
        );
    }
    
    return *this;
}

PhysicsParametersBuilder& PhysicsParametersBuilder::from_existing(
    std::shared_ptr<PhysicsParameters> existing
) {
    if (!existing) {
        throw std::runtime_error("Cannot load from null PhysicsParameters");
    }
    
    *params = *existing;
    
    // Mark as having required parameters
    has_gamma = true;
    has_neighbor_number = true;
    
    return *this;
}

void PhysicsParametersBuilder::validate() const {
    // Validate gamma
    if (params->gamma <= 1.0) {
        throw std::runtime_error(
            "Invalid gamma (" + std::to_string(params->gamma) + 
            "): must be > 1.0 for physical fluids"
        );
    }
    
    // Validate neighbor number
    if (params->neighbor_number <= 0) {
        throw std::runtime_error("Neighbor number must be positive");
    }
    
    // Validate artificial viscosity (if time-dependent)
    if (params->av.use_time_dependent) {
        if (params->av.alpha_max < params->av.alpha_min) {
            throw std::runtime_error(
                "alpha_max (" + std::to_string(params->av.alpha_max) + 
                ") must be >= alpha_min (" + std::to_string(params->av.alpha_min) + ")"
            );
        }
    }
    
    // Validate periodic boundaries
    if (params->periodic.is_valid) {
        for (int i = 0; i < 3; ++i) {
            if (params->periodic.range_max[i] <= params->periodic.range_min[i]) {
                throw std::runtime_error(
                    "Periodic range_max must be > range_min in dimension " + std::to_string(i)
                );
            }
        }
    }
}

bool PhysicsParametersBuilder::is_complete() const {
    return has_gamma && has_neighbor_number;
}

std::string PhysicsParametersBuilder::get_missing_parameters() const {
    std::string missing = "Missing required physics parameters: ";
    bool first = true;
    
    if (!has_gamma) {
        if (!first) missing += ", ";
        missing += "gamma";
        first = false;
    }
    if (!has_neighbor_number) {
        if (!first) missing += ", ";
        missing += "neighbor_number";
        first = false;
    }
    
    return missing;
}

std::shared_ptr<PhysicsParameters> PhysicsParametersBuilder::build() {
    if (!is_complete()) {
        throw std::runtime_error(get_missing_parameters());
    }
    
    validate();
    return params;
}

}

#include "core/sph_parameters_builder.hpp"
#include "core/sph_algorithm_registry.hpp"
#include "exception.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <cmath>

namespace sph
{

SPHParametersBuilder::SPHParametersBuilder() 
    : params(std::make_shared<SPHParameters>()) {
    // Set sensible defaults for optional parameters
    params->time.start = 0.0;
    params->time.energy = 0.01;
    
    params->av.alpha = 1.0;
    params->av.use_balsara_switch = true;
    params->av.use_time_dependent_av = false;
    params->av.alpha_max = 2.0;
    params->av.alpha_min = 0.1;
    params->av.epsilon = 0.2;
    
    params->ac.is_valid = false;
    params->ac.alpha = 1.0;
    
    params->tree.max_level = 20;
    params->tree.leaf_particle_num = 1;
    
    params->iterative_sml = true;
    
    params->periodic.is_valid = false;
    for (std::size_t i = 0; i < DIM; ++i) {
        params->periodic.range_min[i] = 0.0;
        params->periodic.range_max[i] = 1.0;
    }
    
    params->gravity.is_valid = false;
    params->gravity.constant = 1.0;
    params->gravity.theta = 0.5;
    
    params->gsph.is_2nd_order = true;
}

SPHParametersBuilder& SPHParametersBuilder::with_time(real start, real end, real output, real energy) {
    params->time.start = start;
    params->time.end = end;
    params->time.output = output;
    params->time.energy = (energy < 0) ? output : energy;
    has_time = true;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_sph_type(const std::string& type_name) {
    params->type = SPHAlgorithmRegistry::get_type(type_name);
    has_sph_type = true;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_cfl(real sound, real force) {
    params->cfl.sound = sound;
    params->cfl.force = force;
    has_cfl = true;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_physics(int neighbor_number, real gamma) {
    params->physics.neighbor_number = neighbor_number;
    params->physics.gamma = gamma;
    has_physics = true;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_kernel(const std::string& kernel_name) {
    if (kernel_name == "cubic_spline") {
        params->kernel = KernelType::CUBIC_SPLINE;
    } else if (kernel_name == "wendland") {
        params->kernel = KernelType::WENDLAND;
    } else {
        throw std::runtime_error("Unknown kernel type: " + kernel_name);
    }
    has_kernel = true;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_artificial_viscosity(
    real alpha,
    bool use_balsara_switch,
    bool use_time_dependent_av,
    real alpha_max,
    real alpha_min,
    real epsilon
) {
    params->av.alpha = alpha;
    params->av.use_balsara_switch = use_balsara_switch;
    params->av.use_time_dependent_av = use_time_dependent_av;
    params->av.alpha_max = alpha_max;
    params->av.alpha_min = alpha_min;
    params->av.epsilon = epsilon;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_artificial_conductivity(real alpha) {
    params->ac.is_valid = true;
    params->ac.alpha = alpha;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_periodic_boundary(
    const real range_min[DIM],
    const real range_max[DIM]
) {
    params->periodic.is_valid = true;
    for (std::size_t i = 0; i < DIM; ++i) {
        params->periodic.range_min[i] = range_min[i];
        params->periodic.range_max[i] = range_max[i];
    }
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_gravity(real constant, real theta) {
    params->gravity.is_valid = true;
    params->gravity.constant = constant;
    params->gravity.theta = theta;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_tree_params(int max_level, int leaf_particle_num) {
    params->tree.max_level = max_level;
    params->tree.leaf_particle_num = leaf_particle_num;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_iterative_smoothing_length(bool enable) {
    params->iterative_sml = enable;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::with_gsph_2nd_order(bool enable) {
    params->gsph.is_2nd_order = enable;
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::from_json_file(const char* filename) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    pt::read_json(filename, input);
    
    // Time
    if (input.count("startTime") || input.count("endTime") || input.count("outputTime")) {
        real start = input.get<real>("startTime", params->time.start);
        real end = input.get<real>("endTime", 0.0);
        real output = input.get<real>("outputTime", 0.0);
        real energy = input.get<real>("energyTime", -1.0);
        if (end > 0) {
            with_time(start, end, output > 0 ? output : (end - start) / 100, energy);
        }
    }
    
    // SPH Type
    if (input.count("SPHType")) {
        with_sph_type(input.get<std::string>("SPHType"));
    }
    
    // CFL
    if (input.count("cflSound") || input.count("cflForce")) {
        with_cfl(
            input.get<real>("cflSound", 0.3),
            input.get<real>("cflForce", 0.125)
        );
    }
    
    // Physics
    if (input.count("neighborNumber") || input.count("gamma")) {
        with_physics(
            input.get<int>("neighborNumber", 32),
            input.get<real>("gamma", 1.4)
        );
    }
    
    // Kernel
    if (input.count("kernel")) {
        with_kernel(input.get<std::string>("kernel"));
    }
    
    // Artificial Viscosity
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
    
    // Artificial Conductivity
    if (input.get<bool>("useArtificialConductivity", false)) {
        with_artificial_conductivity(input.get<real>("alphaAC", 1.0));
    }
    
    // Periodic boundary
    if (input.get<bool>("periodic", false)) {
        real range_min[DIM];
        real range_max[DIM];
        
        auto& range_min_node = input.get_child("rangeMin");
        auto& range_max_node = input.get_child("rangeMax");
        
        int i = 0;
        for (auto& v : range_min_node) {
            range_min[i++] = std::stod(v.second.data());
        }
        i = 0;
        for (auto& v : range_max_node) {
            range_max[i++] = std::stod(v.second.data());
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
    
    // Tree
    if (input.count("maxTreeLevel") || input.count("leafParticleNumber")) {
        with_tree_params(
            input.get<int>("maxTreeLevel", 20),
            input.get<int>("leafParticleNumber", 1)
        );
    }
    
    // Smoothing length
    if (input.count("iterativeSmoothingLength")) {
        with_iterative_smoothing_length(
            input.get<bool>("iterativeSmoothingLength", true)
        );
    }
    
    // GSPH
    if (input.count("use2ndOrderGSPH")) {
        with_gsph_2nd_order(input.get<bool>("use2ndOrderGSPH", true));
    }
    
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::from_json_string(const char* json_content) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    std::istringstream is(json_content);
    pt::read_json(is, input);
    
    // Reuse file loading logic
    // TODO: Refactor to avoid code duplication
    return *this;
}

SPHParametersBuilder& SPHParametersBuilder::from_existing(std::shared_ptr<SPHParameters> existing) {
    if (!existing) {
        throw std::runtime_error("Cannot load from null SPHParameters");
    }
    
    *params = *existing;
    
    // Mark as having all required parameters
    has_time = true;
    has_sph_type = true;
    has_cfl = true;
    has_physics = true;
    has_kernel = true;
    
    return *this;
}

void SPHParametersBuilder::validate_time() const {
    if (params->time.end < params->time.start) {
        throw std::runtime_error(
            "Invalid time range: endTime (" + std::to_string(params->time.end) + 
            ") < startTime (" + std::to_string(params->time.start) + ")"
        );
    }
    if (params->time.output <= 0) {
        throw std::runtime_error("Output time must be positive");
    }
}

void SPHParametersBuilder::validate_cfl() const {
    if (params->cfl.sound <= 0) {
        throw std::runtime_error("CFL sound speed must be positive");
    }
    if (params->cfl.force <= 0) {
        throw std::runtime_error("CFL force must be positive");
    }
}

void SPHParametersBuilder::validate_physics() const {
    if (params->physics.neighbor_number <= 0) {
        throw std::runtime_error("Neighbor number must be positive");
    }
    if (params->physics.gamma <= 1.0) {
        throw std::runtime_error("Gamma must be > 1.0 for physical fluids");
    }
}

void SPHParametersBuilder::validate_build() const {
    if (!is_complete()) {
        throw std::runtime_error(
            "Cannot build incomplete SPHParameters. " + get_missing_parameters()
        );
    }
    
    validate_time();
    validate_cfl();
    validate_physics();
    
    // Additional validations
    if (params->av.use_time_dependent_av) {
        if (params->av.alpha_max < params->av.alpha_min) {
            throw std::runtime_error("alpha_max must be >= alpha_min");
        }
    }
    
    if (params->periodic.is_valid) {
        for (int i = 0; i < DIM; ++i) {
            if (params->periodic.range_max[i] <= params->periodic.range_min[i]) {
                throw std::runtime_error("Periodic range_max must be > range_min");
            }
        }
    }
}

bool SPHParametersBuilder::is_complete() const {
    return has_time && has_sph_type && has_cfl && has_physics && has_kernel;
}

std::string SPHParametersBuilder::get_missing_parameters() const {
    std::string missing = "Missing required parameters: ";
    bool first = true;
    
    if (!has_time) {
        if (!first) missing += ", ";
        missing += "time";
        first = false;
    }
    if (!has_sph_type) {
        if (!first) missing += ", ";
        missing += "sph_type";
        first = false;
    }
    if (!has_cfl) {
        if (!first) missing += ", ";
        missing += "cfl";
        first = false;
    }
    if (!has_physics) {
        if (!first) missing += ", ";
        missing += "physics";
        first = false;
    }
    if (!has_kernel) {
        if (!first) missing += ", ";
        missing += "kernel";
        first = false;
    }
    
    return missing;
}

std::shared_ptr<SPHParameters> SPHParametersBuilder::build() {
    validate_build();
    return params;
}

}

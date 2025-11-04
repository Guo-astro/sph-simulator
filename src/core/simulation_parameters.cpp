#include "core/simulation_parameters.hpp"
#include "core/sph_algorithm_registry.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdexcept>

namespace sph
{

SimulationParametersBuilder::SimulationParametersBuilder() 
    : params(std::make_shared<SimulationParameters>()) {
    
    // Initialize with defaults
    params->time.start = 0.0;
    params->time.end = 0.0;
    
    params->sph_type = SPHType::SSPH;
    
    params->cfl.sound = 0.3;
    params->cfl.force = 0.125;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_time(real start, real end) {
    params->time.start = start;
    params->time.end = end;
    has_time = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_sph_type(const std::string& type_name) {
    params->sph_type = SPHAlgorithmRegistry::get_type(type_name);
    has_sph_type = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_cfl(real sound, real force) {
    params->cfl.sound = sound;
    params->cfl.force = force;
    has_cfl = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_physics(
    std::shared_ptr<PhysicsParameters> physics
) {
    if (!physics) {
        throw std::runtime_error("Cannot set null PhysicsParameters");
    }
    params->physics = physics;
    has_physics = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_computational(
    std::shared_ptr<ComputationalParameters> computational
) {
    if (!computational) {
        throw std::runtime_error("Cannot set null ComputationalParameters");
    }
    params->computational = computational;
    has_computational = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::with_output(
    std::shared_ptr<OutputParameters> output
) {
    if (!output) {
        throw std::runtime_error("Cannot set null OutputParameters");
    }
    params->output = output;
    has_output = true;
    return *this;
}

SimulationParametersBuilder& SimulationParametersBuilder::from_json_file(const char* filename) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    pt::read_json(filename, input);
    
    // Time
    if (input.count("startTime") && input.count("endTime")) {
        with_time(
            input.get<real>("startTime"),
            input.get<real>("endTime")
        );
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
    
    // Load category parameters from same JSON file
    // Physics parameters
    auto physics_builder = PhysicsParametersBuilder();
    
    if (input.count("gamma")) {
        physics_builder.with_gamma(input.get<real>("gamma"));
    }
    if (input.count("neighborNumber")) {
        physics_builder.with_neighbor_number(input.get<int>("neighborNumber"));
    }
    if (input.count("avAlpha")) {
        physics_builder.with_artificial_viscosity(
            input.get<real>("avAlpha", 1.0),
            input.get<bool>("useBalsaraSwitch", true),
            input.get<bool>("useTimeDependentAV", false),
            input.get<real>("alphaMax", 2.0),
            input.get<real>("alphaMin", 0.1),
            input.get<real>("epsilonAV", 0.2)
        );
    }
    if (input.get<bool>("useArtificialConductivity", false)) {
        physics_builder.with_artificial_conductivity(input.get<real>("alphaAC", 1.0));
    }
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
        
        physics_builder.with_periodic_boundary(range_min, range_max);
    }
    if (input.get<bool>("useGravity", false)) {
        physics_builder.with_gravity(
            input.get<real>("G", 1.0),
            input.get<real>("theta", 0.5)
        );
    }
    
    with_physics(physics_builder.build());
    
    // Computational parameters
    auto comp_builder = ComputationalParametersBuilder();
    
    if (input.count("kernel")) {
        comp_builder.with_kernel(input.get<std::string>("kernel"));
    }
    if (input.count("maxTreeLevel") || input.count("leafParticleNumber")) {
        comp_builder.with_tree_params(
            input.get<int>("maxTreeLevel", 20),
            input.get<int>("leafParticleNumber", 1)
        );
    }
    if (input.count("iterativeSmoothingLength")) {
        comp_builder.with_iterative_smoothing_length(
            input.get<bool>("iterativeSmoothingLength", true)
        );
    }
    if (input.count("use2ndOrderGSPH")) {
        comp_builder.with_gsph_2nd_order(input.get<bool>("use2ndOrderGSPH", true));
    }
    
    with_computational(comp_builder.build());
    
    // Output parameters
    auto output_builder = OutputParametersBuilder();
    
    if (input.count("outputDirectory")) {
        output_builder.with_directory(input.get<std::string>("outputDirectory"));
    }
    if (input.count("outputTime")) {
        output_builder.with_particle_output_interval(input.get<real>("outputTime"));
    }
    if (input.count("energyTime")) {
        output_builder.with_energy_output_interval(input.get<real>("energyTime"));
    }
    
    with_output(output_builder.build());
    
    return *this;
}

void SimulationParametersBuilder::validate() const {
    // Validate time range
    if (params->time.end <= params->time.start) {
        throw std::runtime_error(
            "Invalid time range: end (" + std::to_string(params->time.end) + 
            ") <= start (" + std::to_string(params->time.start) + ")"
        );
    }
    
    // Validate CFL
    if (params->cfl.sound <= 0) {
        throw std::runtime_error("CFL sound must be positive");
    }
    if (params->cfl.force <= 0) {
        throw std::runtime_error("CFL force must be positive");
    }
    
    // Category parameters are already validated in their own builders
}

bool SimulationParametersBuilder::is_complete() const {
    return has_time && has_sph_type && has_cfl && 
           has_physics && has_computational && has_output;
}

std::string SimulationParametersBuilder::get_missing_parameters() const {
    std::string missing = "Missing required simulation parameters: ";
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
    if (!has_computational) {
        if (!first) missing += ", ";
        missing += "computational";
        first = false;
    }
    if (!has_output) {
        if (!first) missing += ", ";
        missing += "output";
        first = false;
    }
    
    return missing;
}

std::shared_ptr<SimulationParameters> SimulationParametersBuilder::build() {
    if (!is_complete()) {
        throw std::runtime_error(get_missing_parameters());
    }
    
    validate();
    return params;
}

}

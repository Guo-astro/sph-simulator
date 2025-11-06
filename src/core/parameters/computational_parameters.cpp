#include "core/parameters/computational_parameters.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdexcept>

namespace sph
{

ComputationalParametersBuilder::ComputationalParametersBuilder() 
    : params(std::make_shared<ComputationalParameters>()) {
    
    // Set sensible defaults for all computational parameters
    
    // Default kernel
    params->kernel = KernelType::CUBIC_SPLINE;
    
    // Default tree settings (reasonable for most cases)
    params->tree.max_level = 20;
    params->tree.leaf_particle_num = 1;
    
    // Default smoothing length method
    params->iterative_smoothing_length = true;
    
    // Default GSPH settings
    params->gsph.is_2nd_order = true;
}

ComputationalParametersBuilder& ComputationalParametersBuilder::with_kernel(
    const std::string& kernel_name
) {
    if (kernel_name == "cubic_spline") {
        params->kernel = KernelType::CUBIC_SPLINE;
    } else if (kernel_name == "wendland") {
        params->kernel = KernelType::WENDLAND;
    } else {
        throw std::runtime_error(
            "Unknown kernel type: '" + kernel_name + "'. " +
            "Available kernels: cubic_spline, wendland"
        );
    }
    return *this;
}

ComputationalParametersBuilder& ComputationalParametersBuilder::with_tree_params(
    int max_level,
    int leaf_particle_num
) {
    params->tree.max_level = max_level;
    params->tree.leaf_particle_num = leaf_particle_num;
    return *this;
}

ComputationalParametersBuilder& ComputationalParametersBuilder::with_iterative_smoothing_length(
    bool enable
) {
    params->iterative_smoothing_length = enable;
    return *this;
}

ComputationalParametersBuilder& ComputationalParametersBuilder::with_gsph_2nd_order(bool enable) {
    params->gsph.is_2nd_order = enable;
    return *this;
}

ComputationalParametersBuilder& ComputationalParametersBuilder::from_json(const char* filename) {
    namespace pt = boost::property_tree;
    pt::ptree input;
    pt::read_json(filename, input);
    
    // Kernel
    if (input.count("kernel")) {
        with_kernel(input.get<std::string>("kernel"));
    }
    
    // Tree parameters
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

ComputationalParametersBuilder& ComputationalParametersBuilder::from_existing(
    std::shared_ptr<ComputationalParameters> existing
) {
    if (!existing) {
        throw std::runtime_error("Cannot load from null ComputationalParameters");
    }
    
    *params = *existing;
    return *this;
}

void ComputationalParametersBuilder::validate() const {
    // Validate tree parameters
    if (params->tree.max_level < 1) {
        throw std::runtime_error(
            "Tree max_level must be >= 1, got " + std::to_string(params->tree.max_level)
        );
    }
    
    if (params->tree.leaf_particle_num < 1) {
        throw std::runtime_error(
            "Tree leaf_particle_num must be >= 1, got " + 
            std::to_string(params->tree.leaf_particle_num)
        );
    }
    
    // Validate kernel
    if (params->kernel == KernelType::UNKNOWN) {
        throw std::runtime_error("Kernel type must be set");
    }
}

std::shared_ptr<ComputationalParameters> ComputationalParametersBuilder::build() {
    validate();
    return params;
}

}

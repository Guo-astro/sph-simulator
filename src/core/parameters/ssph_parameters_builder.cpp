#include "core/parameters/ssph_parameters_builder.hpp"
#include "exception.hpp"
#include <sstream>

namespace sph {

AlgorithmParametersBuilder<SSPHTag>::AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params)
    : params(base_params) {
    // Set default artificial viscosity values
    params->av.alpha = 1.0;
    params->av.use_balsara_switch = true;
    params->av.use_time_dependent_av = false;
    params->av.alpha_max = 2.0;
    params->av.alpha_min = 0.1;
    params->av.epsilon = 0.2;
    
    // Artificial conductivity disabled by default
    params->ac.is_valid = false;
    params->ac.alpha = 1.0;
}

void AlgorithmParametersBuilder<SSPHTag>::validate_build() const {
    if (!has_artificial_viscosity) {
        throw std::runtime_error(
            "SSPH requires artificial viscosity. "
            "Call with_artificial_viscosity() before build()."
        );
    }
    
    // Validate viscosity parameters
    if (params->av.alpha < 0.0) {
        throw std::runtime_error("Artificial viscosity alpha must be non-negative.");
    }
    
    if (params->av.use_time_dependent_av) {
        if (params->av.alpha_max <= params->av.alpha_min) {
            throw std::runtime_error("alpha_max must be greater than alpha_min.");
        }
        if (params->av.epsilon <= 0.0) {
            throw std::runtime_error("epsilon must be positive.");
        }
    }
}

AlgorithmParametersBuilder<SSPHTag>& AlgorithmParametersBuilder<SSPHTag>::with_artificial_viscosity(
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
    has_artificial_viscosity = true;
    return *this;
}

AlgorithmParametersBuilder<SSPHTag>& AlgorithmParametersBuilder<SSPHTag>::with_artificial_conductivity(real alpha) {
    params->ac.is_valid = true;
    params->ac.alpha = alpha;
    return *this;
}

std::shared_ptr<SPHParameters> AlgorithmParametersBuilder<SSPHTag>::build() {
    validate_build();
    return params;
}

bool AlgorithmParametersBuilder<SSPHTag>::is_complete() const {
    return has_artificial_viscosity;
}

std::string AlgorithmParametersBuilder<SSPHTag>::get_missing_parameters() const {
    if (!has_artificial_viscosity) {
        return "Missing required parameter: artificial_viscosity";
    }
    return "";
}

} // namespace sph

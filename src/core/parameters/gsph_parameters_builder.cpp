#include "core/parameters/gsph_parameters_builder.hpp"
#include "exception.hpp"
#include <sstream>

namespace sph {

AlgorithmParametersBuilder<GSPHTag>::AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params)
    : params(base_params) {
    // GSPH defaults - NO artificial viscosity
    // Set 2nd order MUSCL to false by default (1st order is more stable)
    params->gsph.is_2nd_order = false;
}

void AlgorithmParametersBuilder<GSPHTag>::validate_build() const {
    // GSPH has no additional required parameters beyond base
    // Riemann solver is always enabled for GSPH
    // No artificial viscosity required - validation pass
}

AlgorithmParametersBuilder<GSPHTag>& AlgorithmParametersBuilder<GSPHTag>::with_2nd_order_muscl(bool enable) {
    params->gsph.is_2nd_order = enable;
    return *this;
}

std::shared_ptr<SPHParameters> AlgorithmParametersBuilder<GSPHTag>::build() {
    validate_build();
    return params;
}

bool AlgorithmParametersBuilder<GSPHTag>::is_complete() const {
    // GSPH has no additional required parameters
    return true;
}

std::string AlgorithmParametersBuilder<GSPHTag>::get_missing_parameters() const {
    // GSPH has no additional required parameters
    return "";
}

} // namespace sph

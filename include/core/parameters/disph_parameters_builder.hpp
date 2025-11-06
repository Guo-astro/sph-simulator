#ifndef SPH_SIMULATION_DISPH_PARAMETERS_BUILDER_HPP
#define SPH_SIMULATION_DISPH_PARAMETERS_BUILDER_HPP

#include "sph_parameters_builder_base.hpp"
#include "../algorithms/algorithm_tags.hpp"
#include <memory>

namespace sph {

/// Algorithm-specific parameter builder for Density Independent SPH (DISPH)
/// DISPH REQUIRES artificial viscosity for shock capturing and stability
/// Uses density-independent formulation but still needs viscosity
template <>
class AlgorithmParametersBuilder<DISPHTag> {
private:
    std::shared_ptr<SPHParameters> params;
    bool has_artificial_viscosity = false;
    
    void validate_build() const;
    
public:
    // Constructor takes ownership of base parameters
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params);
    
    /// Set artificial viscosity parameters (REQUIRED for DISPH)
    /// @param alpha Viscosity coefficient (typically 1.0-2.0)
    /// @param use_balsara_switch Use Balsara switch to reduce viscosity in shear flows
    /// @param use_time_dependent_av Use time-dependent artificial viscosity
    /// @param alpha_max Maximum alpha for time-dependent viscosity (default 2.0)
    /// @param alpha_min Minimum alpha for time-dependent viscosity (default 0.1)
    /// @param epsilon Small number to avoid division by zero (default 0.2)
    AlgorithmParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent_av = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    );
    
    /// Optional: Set artificial conductivity for energy equation
    /// @param alpha Conductivity coefficient (typically same as viscosity alpha)
    AlgorithmParametersBuilder& with_artificial_conductivity(real alpha);
    
    /// Build final parameters for DISPH
    /// @throws std::runtime_error if artificial viscosity not set
    std::shared_ptr<SPHParameters> build();
    
    /// Check if all required DISPH parameters are set
    bool is_complete() const;
    
    /// Get list of missing required parameters
    std::string get_missing_parameters() const;
};

} // namespace sph

#endif // SPH_SIMULATION_DISPH_PARAMETERS_BUILDER_HPP

#ifndef SPH_SIMULATION_GSPH_PARAMETERS_BUILDER_HPP
#define SPH_SIMULATION_GSPH_PARAMETERS_BUILDER_HPP

#include "sph_parameters_builder_base.hpp"
#include "../algorithms/algorithm_tags.hpp"
#include <memory>

namespace sph {

/// Algorithm-specific parameter builder for Godunov SPH (GSPH)
/// GSPH uses Riemann solver for shock capturing, NOT artificial viscosity
/// This builder DOES NOT provide .with_artificial_viscosity() method
/// Type safety enforced: attempting to set viscosity will cause compile error
template <>
class AlgorithmParametersBuilder<GSPHTag> {
private:
    std::shared_ptr<SPHParameters> params;
    
    void validate_build() const;
    
public:
    // Constructor takes ownership of base parameters
    explicit AlgorithmParametersBuilder(std::shared_ptr<SPHParameters> base_params);
    
    /// Enable 2nd order MUSCL scheme for GSPH (optional)
    /// Uses slope limiter (Van Leer) for spatial reconstruction
    /// @param enable Whether to use 2nd order reconstruction (default false for 1st order)
    AlgorithmParametersBuilder& with_2nd_order_muscl(bool enable = true);
    
    // NOTE: NO with_artificial_viscosity() method - GSPH does not use it!
    // GSPH relies on Riemann solver (HLL) for shock capturing
    
    /// Build final parameters for GSPH
    std::shared_ptr<SPHParameters> build();
    
    /// Check if all required GSPH parameters are set
    /// GSPH has no additional required parameters beyond base
    bool is_complete() const;
    
    /// Get list of missing required parameters
    std::string get_missing_parameters() const;
};

} // namespace sph

#endif // SPH_SIMULATION_GSPH_PARAMETERS_BUILDER_HPP

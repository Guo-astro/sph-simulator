#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include "parameters.hpp"

namespace sph
{

/**
 * @brief Fluent builder for type-safe SPHParameters construction.
 * 
 * This class provides compile-time and runtime safety for parameter creation:
 * - Fluent interface for clarity
 * - Validation of parameter values
 * - Clear error messages for missing required parameters
 * - Support for JSON loading
 * - Method chaining for ergonomic API
 * 
 * Required parameters:
 * - time (start, end, output)
 * - sph_type
 * - cfl (sound, force)
 * - physics (neighbor_number, gamma)
 * - kernel
 * 
 * Optional parameters:
 * - artificial_viscosity
 * - artificial_conductivity
 * - periodic_boundary
 * - gravity
 * - tree parameters
 * - GSPH-specific settings
 * 
 * Usage:
 *   auto params = SPHParametersBuilder()
 *       .with_time(0.0, 0.2, 0.01)
 *       .with_sph_type("gsph")
 *       .with_cfl(0.3, 0.125)
 *       .with_physics(50, 1.4)
 *       .with_kernel("cubic_spline")
 *       .with_periodic_boundary(range_min, range_max)
 *       .build();
 */
class SPHParametersBuilder {
private:
    std::shared_ptr<SPHParameters> params;
    
    // Tracking which required parameters have been set
    bool has_time = false;
    bool has_sph_type = false;
    bool has_cfl = false;
    bool has_physics = false;
    bool has_kernel = false;
    
    void validate_build() const;
    void validate_time() const;
    void validate_cfl() const;
    void validate_physics() const;
    
public:
    SPHParametersBuilder();
    
    // Required parameters
    SPHParametersBuilder& with_time(real start, real end, real output, real energy = -1.0);
    SPHParametersBuilder& with_sph_type(const std::string& type_name);
    SPHParametersBuilder& with_cfl(real sound, real force);
    SPHParametersBuilder& with_physics(int neighbor_number, real gamma);
    SPHParametersBuilder& with_kernel(const std::string& kernel_name);
    
    // Optional parameters
    SPHParametersBuilder& with_artificial_viscosity(
        real alpha,
        bool use_balsara_switch = true,
        bool use_time_dependent_av = false,
        real alpha_max = 2.0,
        real alpha_min = 0.1,
        real epsilon = 0.2
    );
    
    SPHParametersBuilder& with_artificial_conductivity(real alpha);
    
    SPHParametersBuilder& with_periodic_boundary(
        const real range_min[DIM],
        const real range_max[DIM]
    );
    
    SPHParametersBuilder& with_gravity(real constant, real theta = 0.5);
    
    SPHParametersBuilder& with_tree_params(int max_level = 20, int leaf_particle_num = 1);
    
    SPHParametersBuilder& with_iterative_smoothing_length(bool enable = true);
    
    SPHParametersBuilder& with_gsph_2nd_order(bool enable = true);
    
    // JSON loading
    SPHParametersBuilder& from_json_file(const char* filename);
    SPHParametersBuilder& from_json_string(const char* json_content);
    
    // Merge with existing parameters
    SPHParametersBuilder& from_existing(std::shared_ptr<SPHParameters> existing);
    
    // Build final parameters
    std::shared_ptr<SPHParameters> build();
    
    // Helper to check if all required parameters are set
    bool is_complete() const;
    std::string get_missing_parameters() const;
};

}

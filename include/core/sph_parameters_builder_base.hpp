#ifndef SPH_SIMULATION_SPH_PARAMETERS_BUILDER_BASE_HPP
#define SPH_SIMULATION_SPH_PARAMETERS_BUILDER_BASE_HPP

#include "algorithm_tags.hpp"
#include "parameters.hpp"
#include <memory>
#include <string>
#include <array>

namespace sph {

// Forward declarations of algorithm-specific builders
template <typename AlgorithmTag> class AlgorithmParametersBuilder;

/// Base parameter builder for common SPH parameters shared across all algorithms
/// Handles: time, CFL, physics, kernel, gravity, tree, periodic boundaries
/// Provides transition methods .as_ssph(), .as_disph(), .as_gsph() to algorithm-specific builders
class SPHParametersBuilderBase {
protected:
    std::shared_ptr<SPHParameters> params;
    
    // Tracking required parameters
    bool has_time = false;
    bool has_cfl = false;
    bool has_physics = false;
    bool has_kernel = false;
    
    void validate_time() const;
    void validate_cfl() const;
    void validate_physics() const;
    void validate_kernel() const;
    
public:
    SPHParametersBuilderBase();
    virtual ~SPHParametersBuilderBase() = default;
    
    // Required common parameters
    
    /// Set simulation time parameters
    /// @param start Start time (typically 0.0)
    /// @param end End time for simulation
    /// @param output Output interval for snapshots
    /// @param energy Energy output interval (default -1.0 means use output interval)
    SPHParametersBuilderBase& with_time(real start, real end, real output, real energy = -1.0);
    
    /// Set CFL (Courant-Friedrichs-Lewy) condition parameters
    /// @param sound CFL number for sound speed (typically 0.3-0.4)
    /// @param force CFL number for force term (typically 0.25)
    SPHParametersBuilderBase& with_cfl(real sound, real force);
    
    /// Set physical parameters
    /// @param neighbor_number Target number of neighbors for kernel support
    /// @param gamma Adiabatic index (5/3 for monoatomic ideal gas)
    SPHParametersBuilderBase& with_physics(int neighbor_number, real gamma);
    
    /// Set kernel function type
    /// @param kernel_name Name of kernel (e.g., "cubic_spline", "wendland_c2", "wendland_c4")
    SPHParametersBuilderBase& with_kernel(const std::string& kernel_name);
    
    // Optional common parameters
    
    /// Enable gravity and Barnes-Hut tree parameters
    /// @param constant Gravitational constant
    /// @param theta Opening angle for Barnes-Hut approximation (default 0.5)
    SPHParametersBuilderBase& with_gravity(real constant, real theta = 0.5);
    
    /// Set tree construction parameters
    /// @param max_level Maximum tree depth (default 20)
    /// @param leaf_particle_num Particles per leaf node (default 1)
    SPHParametersBuilderBase& with_tree_params(int max_level = 20, int leaf_particle_num = 1);
    
    /// Enable periodic boundary conditions
    /// @param range_min Minimum coordinates for periodic box
    /// @param range_max Maximum coordinates for periodic box
    SPHParametersBuilderBase& with_periodic_boundary(
        const std::array<real, 3>& range_min,
        const std::array<real, 3>& range_max
    );
    
    /// Enable iterative smoothing length solver
    /// @param enable Whether to use iterative method (default true)
    SPHParametersBuilderBase& with_iterative_smoothing_length(bool enable = true);
    
    // JSON loading - common for all algorithms
    
    /// Load parameters from JSON file
    SPHParametersBuilderBase& from_json_file(const char* filename);
    
    /// Load parameters from JSON string
    SPHParametersBuilderBase& from_json_string(const char* json_content);
    
    /// Merge with existing parameters
    SPHParametersBuilderBase& from_existing(std::shared_ptr<SPHParameters> existing);
    
    // Transition to algorithm-specific builders
    
    /// Transition to Standard SPH (SSPH) builder
    /// SSPH uses artificial viscosity and standard momentum equation
    AlgorithmParametersBuilder<SSPHTag> as_ssph();
    
    /// Transition to Density Independent SPH (DISPH) builder  
    /// DISPH uses artificial viscosity with density-independent formulation
    AlgorithmParametersBuilder<DISPHTag> as_disph();
    
    /// Transition to Godunov SPH (GSPH) builder
    /// GSPH uses Riemann solver for shock capturing
    AlgorithmParametersBuilder<GSPHTag> as_gsph();
    
    // Utility methods
    
    /// Check if all required common parameters are set
    bool is_complete() const;
    
    /// Get list of missing required parameters
    std::string get_missing_parameters() const;
    
    /// Get current parameter object (for internal use by derived builders)
    std::shared_ptr<SPHParameters> get_params() const { return params; }
};

} // namespace sph

#endif // SPH_SIMULATION_SPH_PARAMETERS_BUILDER_BASE_HPP

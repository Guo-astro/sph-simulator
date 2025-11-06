#pragma once

/**
 * @file computational_parameters.hpp
 * @brief Computational/algorithmic parameters for SPH simulation
 * 
 * This file contains ONLY parameters that affect HOW the simulation is computed:
 * - Kernel function choice
 * - Tree algorithm settings (for neighbor search)
 * - Smoothing length iteration method
 * - GSPH-specific computational options
 * 
 * These parameters determine HOW the physics is computed, not WHAT physics.
 */

#include "../../defines.hpp"
#include "core/particles/sph_types.hpp"
#include <memory>

namespace sph
{

/**
 * @brief Computational/algorithmic parameters
 * 
 * These parameters control the numerical methods and algorithms used.
 */
struct ComputationalParameters {
    
    // Kernel function
    KernelType kernel;
    
    // Tree algorithm settings (for efficient neighbor search)
    struct Tree {
        int max_level;           ///< Maximum tree depth
        int leaf_particle_num;   ///< Particles per leaf node
    } tree;
    
    // Smoothing length computation
    bool iterative_smoothing_length;  ///< Use iterative h adjustment
    
    // Algorithm-specific settings
    struct GSPH {
        bool is_2nd_order;  ///< Use 2nd order accurate GSPH
    } gsph;
};

/**
 * @brief Builder for computational parameters with sensible defaults
 */
class ComputationalParametersBuilder {
public:
    ComputationalParametersBuilder();
    
    // Kernel selection
    ComputationalParametersBuilder& with_kernel(const std::string& kernel_name);
    
    // Tree algorithm tuning
    ComputationalParametersBuilder& with_tree_params(int max_level, int leaf_particle_num);
    
    // Smoothing length method
    ComputationalParametersBuilder& with_iterative_smoothing_length(bool enable);
    
    // Algorithm-specific options
    ComputationalParametersBuilder& with_gsph_2nd_order(bool enable);
    
    // Load from configuration
    ComputationalParametersBuilder& from_json(const char* filename);
    ComputationalParametersBuilder& from_existing(std::shared_ptr<ComputationalParameters> existing);
    
    // Build with validation
    std::shared_ptr<ComputationalParameters> build();
    
private:
    std::shared_ptr<ComputationalParameters> params;
    
    void validate() const;
};

}

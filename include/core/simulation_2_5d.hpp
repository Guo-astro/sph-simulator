#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "sph_particle_2_5d.hpp"
#include "cubic_spline_2_5d.hpp"
#include "bhtree_2_5d.hpp"
#include "../defines.hpp"

namespace sph {

struct SPHParameters;

/**
 * @brief 2.5D SPH Simulation class
 * 
 * Combines 2D hydrodynamics (r-z plane) with 3D gravity calculations.
 * Assumes azimuthal symmetry for hydrodynamic forces but full 3D gravity.
 */
class Simulation25D {
public:
    std::vector<SPHParticle25D> particles;
    int particle_num;
    real time;
    real dt;
    real h_per_v_sig;
    
    // 2D kernel for hydrodynamics
    std::shared_ptr<Cubic25D> kernel;
    
    // 3D tree for gravity
    std::shared_ptr<BHTree25D> tree;
    
    std::unordered_map<std::string, std::vector<real>> additional_scalar_array;
    std::unordered_map<std::string, std::vector<Vector<2>>> additional_vector_array;

    Simulation25D(std::shared_ptr<SPHParameters> param);
    
    void update_time();
    void make_tree();
    void add_scalar_array(const std::vector<std::string>& names);
    void add_vector_array(const std::vector<std::string>& names);
    std::vector<real>& get_scalar_array(const std::string& name);
    std::vector<Vector<2>>& get_vector_array(const std::string& name);
    
    /**
     * @brief Update gravity positions for all particles
     * @param azimuthal_angles Optional azimuthal angles for each particle
     */
    void update_gravity_positions(const std::vector<real>& azimuthal_angles = {});
    
    /**
     * @brief Calculate gravitational forces using 3D tree
     */
    void calculate_gravity();
    
    /**
     * @brief Calculate hydrodynamic forces using 2D kernel
     */
    void calculate_hydrodynamics();
    
    /**
     * @brief Full timestep: gravity + hydrodynamics + time integration
     */
    void timestep();
};

} // namespace sph

#include "simulation_2_5d.tpp"</content>
<parameter name="filePath">/Users/guo/sph-simulation/include/core/simulation_2_5d.hpp
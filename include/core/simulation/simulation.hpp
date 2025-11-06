#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "../utilities/vector.hpp"
#include "../particles/sph_particle.hpp"
#include "../kernels/kernel_function.hpp"
#include "../../defines.hpp"

namespace sph {

// Forward declarations
template<int Dim>
class BHTree;

template<int Dim>
class Periodic;

template<int Dim>
class GhostParticleManager;

struct SPHParameters;

template<int Dim>
class Simulation {
public:
    std::vector<SPHParticle<Dim>> particles;
    int particle_num;
    real time;
    real dt;
    real h_per_v_sig;
    std::shared_ptr<KernelFunction<Dim>> kernel;
    std::shared_ptr<Periodic<Dim>> periodic;  // Legacy - for backward compatibility
    std::shared_ptr<BHTree<Dim>> tree;
    std::shared_ptr<GhostParticleManager<Dim>> ghost_manager;  // New ghost particle system

    // Cached combined particles for tree building (real + ghost)
    // Must be persistent so tree can store valid pointers
    std::vector<SPHParticle<Dim>> cached_search_particles;

    std::unordered_map<std::string, std::vector<real>> additional_scalar_array;
    std::unordered_map<std::string, std::vector<Vector<Dim>>> additional_vector_array;

    Simulation(std::shared_ptr<SPHParameters> param);
    void update_time();
    void make_tree();
    void add_scalar_array(const std::vector<std::string>& names);
    void add_vector_array(const std::vector<std::string>& names);
    std::vector<real>& get_scalar_array(const std::string& name);
    std::vector<Vector<Dim>>& get_vector_array(const std::string& name);
    
    /**
     * @brief Get combined particles for neighbor search (real + ghost)
     * 
     * Returns a view combining real and ghost particles. Used for neighbor search.
     * The returned vector has real particles at indices [0, particle_num) and
     * ghost particles at indices [particle_num, total_count).
     * 
     * @note This creates a temporary combined vector. Consider caching if called frequently.
     */
    std::vector<SPHParticle<Dim>> get_all_particles_for_search() const;
    
    /**
     * @brief Get total particle count including ghosts
     * 
     * @return Total number of particles (real + ghost)
     */
    int get_total_particle_count() const;
    
    /**
     * @brief Get number of real particles (excludes ghosts)
     * 
     * @return Number of real particles
     */
    int get_real_particle_count() const { return particle_num; }
    
    /**
     * @brief Check if a particle index refers to a real particle
     * 
     * @param index Particle index
     * @return true if index refers to a real particle, false if ghost
     */
    bool is_real_particle(int index) const { return index < particle_num; }
};

// Type aliases for convenience
using Simulation1D = Simulation<1>;
using Simulation2D = Simulation<2>;
using Simulation3D = Simulation<3>;

} // namespace sph

#include "simulation.tpp"

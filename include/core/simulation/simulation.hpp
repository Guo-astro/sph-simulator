#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "../utilities/vector.hpp"
#include "../particles/sph_particle.hpp"
#include "../kernels/kernel_function.hpp"
#include "../neighbors/particle_array_types.hpp"
#include "../neighbors/neighbor_accessor.hpp"
#include "particle_cache.hpp"
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

    // Type-safe particle cache for neighbor search
    // Manages synchronization between real particles and search cache
    ParticleCache<Dim> particle_cache;
    
    // Legacy field - kept for backward compatibility with tree building
    // TODO: Remove once tree is refactored to use particle_cache
    std::vector<SPHParticle<Dim>> cached_search_particles;

    std::unordered_map<std::string, std::vector<real>> additional_scalar_array;
    std::unordered_map<std::string, std::vector<Vector<Dim>>> additional_vector_array;

    Simulation(std::shared_ptr<SPHParameters> param);
    void update_time();
    void make_tree();
    
    /**
     * @brief Synchronize particle cache with current real particle state
     * 
     * Call this after any operation that modifies particle properties
     * (e.g., after pre_interaction, before fluid_force).
     * 
     * This is the declarative, type-safe replacement for manual cache updates.
     */
    void sync_particle_cache();
    
    /**
     * @brief Include ghost particles in search cache
     * 
     * Call this after ghost particles are generated and before tree rebuild.
     */
    void extend_cache_with_ghosts();
    
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
    
    // ========================================================================
    // Type-Safe Neighbor Access API
    // ========================================================================
    
    /**
     * @brief Get type-safe wrapper for real particles only (no ghosts)
     * 
     * Returns a typed wrapper that prevents accidental use with neighbor indices.
     * Use this when iterating over or updating real particles directly.
     * 
     * @return RealParticleArray<Dim> wrapper around particles vector
     */
    RealParticleArray<Dim> get_real_particles() {
        return RealParticleArray<Dim>{particles};
    }
    
    /**
     * @brief Get type-safe wrapper for search particles (real + ghost)
     * 
     * Returns a typed wrapper that is REQUIRED for neighbor access.
     * Neighbor indices reference this array, not the real particles array.
     * 
     * IMPORTANT: Always use NeighborAccessor to access elements by neighbor index.
     * Direct indexing is prevented by the type system.
     * 
     * @return SearchParticleArray<Dim> wrapper around cached_search_particles
     */
    SearchParticleArray<Dim> get_search_particles() {
        return SearchParticleArray<Dim>{cached_search_particles};
    }
    
    /**
     * @brief Create type-safe neighbor accessor
     * 
     * Returns an accessor that enforces:
     * - Neighbor indices ONLY access SearchParticleArray (real + ghost)
     * - Compile-time error if you try to use RealParticleArray
     * - Debug builds: runtime bounds checking with exceptions
     * 
     * Example usage:
     *   auto accessor = sim->create_neighbor_accessor();
     *   for (auto neighbor_idx : result) {
     *       const auto& p_j = accessor.get_neighbor(neighbor_idx);
     *       // ... computation
     *   }
     * 
     * @return NeighborAccessor<Dim> for type-safe neighbor access
     */
    NeighborAccessor<Dim> create_neighbor_accessor() {
        return NeighborAccessor<Dim>{get_search_particles()};
    }
    
    /**
     * @brief Validate particle array invariants (debug builds only)
     * 
     * Checks:
     * - cached_search_particles.size() >= particles.size()
     * - Search particles include all real particles
     * 
     * Throws std::logic_error if invariants violated.
     * In release builds (NDEBUG defined), this is a no-op for performance.
     * 
     * Call this at the entry of SPH calculation methods to catch bugs early.
     */
    void validate_particle_arrays() const {
        #ifndef NDEBUG
        if (cached_search_particles.size() < static_cast<size_t>(particle_num)) {
            throw std::logic_error(
                "Particle array invariant violated: "
                "cached_search_particles (" + std::to_string(cached_search_particles.size()) + 
                ") must include all real particles (" + std::to_string(particle_num) + ")"
            );
        }
        #endif
    }
};

// Type aliases for convenience
using Simulation1D = Simulation<1>;
using Simulation2D = Simulation<2>;
using Simulation3D = Simulation<3>;

} // namespace sph

#include "simulation.tpp"

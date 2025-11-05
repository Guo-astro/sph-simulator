/**
 * @file spatial_tree_coordinator.hpp
 * @brief Coordinator for spatial tree lifecycle and container consistency management
 * 
 * Orchestrates the complex interaction between:
 * - Real particle container (Simulation::particles)
 * - Ghost particle container (GhostParticleManager::ghost_particles)
 * - Search particle container (Simulation::cached_search_particles)
 * - Spatial tree (BHTree)
 * 
 * Key responsibilities:
 * - Synchronize cached_search_particles with real + ghost particles
 * - Manage container capacity to avoid reallocation (prevents tree pointer invalidation)
 * - Clear stale linked-list pointers before tree rebuild
 * - Rebuild spatial tree with consistent container
 * - Validate tree-container consistency
 * 
 * Design principles:
 * - Atomicity: All four steps (sync, clear, validate, rebuild) happen together
 * - Safety: Prevents reallocation that would invalidate tree pointers
 * - Consistency: Enforces invariant that particle.id == container index
 * - Performance: Minimizes reallocations through buffer management
 * 
 * Critical invariants enforced:
 * 1. Tree always built with cached_search_particles
 * 2. Neighbor indices always reference cached_search_particles
 * 3. Particle ID equals its index in cached_search_particles
 * 4. Container never reallocates during tree lifetime
 * 
 * @see BHTree
 * @see Simulation
 * @see GhostParticleManager
 */

#pragma once

#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/bhtree.hpp"
#include "logger.hpp"
#include <memory>
#include <vector>
#include <algorithm>

namespace sph {

/**
 * @brief Coordinator for managing spatial tree and search container consistency
 * 
 * Encapsulates the multi-step process of rebuilding the spatial tree:
 * 1. Synchronize cached_search_particles with real + ghost particles
 * 2. Clear particle.next pointers to prevent stale linked-list references
 * 3. Validate container consistency
 * 4. Rebuild spatial tree
 * 
 * This coordination is critical because:
 * - Tree stores pointers to particles in cached_search_particles
 * - Reallocation invalidates those pointers (segfault)
 * - Stale next pointers cause corruption in tree structure
 * - ID mismatches cause out-of-bounds access during neighbor search
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class SpatialTreeCoordinator {
public:
    /**
     * @brief Construct coordinator for given simulation
     * @param sim Shared pointer to simulation (must not be null)
     * @throws std::invalid_argument if sim is null
     */
    explicit SpatialTreeCoordinator(std::shared_ptr<Simulation<Dim>> sim);
    
    /**
     * @brief Rebuild tree with combined real+ghost particles
     * 
     * This is the main entry point. It performs all four steps atomically:
     * 1. Sync container (real + renumbered ghost particles)
     * 2. Clear next pointers
     * 3. Validate consistency
     * 4. Rebuild tree
     * 
     * Call this:
     * - After ghost initialization
     * - After ghost updates
     * - Before any force calculations that use neighbor search
     * 
     * @note This function modifies cached_search_particles and rebuilds tree
     */
    void rebuild_tree_for_neighbor_search();
    
    /**
     * @brief Get current search particle count
     * @return Size of cached_search_particles (real + ghost)
     */
    size_t get_search_particle_count() const;
    
    /**
     * @brief Validate tree consistency with container
     * @return true if tree built with current cached_search_particles
     */
    bool is_tree_consistent() const;
    
    /// Buffer size for reallocation avoidance
    /// When capacity insufficient, reserve current_size + REALLOCATION_BUFFER
    static constexpr size_t REALLOCATION_BUFFER = 100;

private:
    /**
     * @brief Synchronize cached_search_particles with real + ghost particles
     * 
     * Gets combined particle list from Simulation::get_all_particles_for_search(),
     * which handles ghost ID renumbering. Manages capacity to avoid reallocation.
     * 
     * Ensures: particle[i].id == i for all particles in cached_search_particles
     */
    void synchronize_search_container();
    
    /**
     * @brief Clear particle.next pointers in cached_search_particles
     * 
     * Tree builder (BHNode::assign) modifies particle.next to create linked lists.
     * These pointers become stale when particles are copied to cached_search_particles.
     * Must clear before tree rebuild to prevent corruption.
     */
    void clear_linked_list_pointers();
    
    /**
     * @brief Rebuild the spatial tree with cached_search_particles
     * 
     * Calls BHTree::make() with cached_search_particles container.
     * Tree stores pointer to this container for validation in neighbor_search().
     */
    void rebuild_spatial_tree();
    
    /**
     * @brief Validate cached_search_particles has correct particle IDs
     * 
     * Ensures particle.id == index for all particles.
     * This is critical for tree neighbor search correctness.
     * 
     * @throws std::logic_error if any particle has id != index
     */
    void validate_particle_ids() const;
    
    /// Shared pointer to simulation (never null after construction)
    std::shared_ptr<Simulation<Dim>> m_sim;
};

} // namespace sph

#include "core/spatial_tree_coordinator.tpp"

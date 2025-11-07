#pragma once

#include "../particles/sph_particle.hpp"
#include <vector>
#include <memory>

namespace sph {

template<int Dim>
class GhostParticleManager;

/**
 * @brief Type-safe particle cache for neighbor search operations
 * 
 * Manages the synchronization between real particles and search cache.
 * Ensures cached particles always reflect the latest state of real particles
 * before neighbor search operations.
 * 
 * Design Principles:
 * - Declarative API: sync() instead of manual loops
 * - Type-safe: No raw index manipulation
 * - Single Responsibility: Only manages cache synchronization
 * - Testable: Clear preconditions and postconditions
 */
template<int Dim>
class ParticleCache {
public:
    /**
     * @brief Initialize cache with real particles
     * 
     * Preconditions:
     * - real_particles must not be empty
     * 
     * Postconditions:
     * - cache size equals real particle count
     * - cache contains copies of real particles
     */
    void initialize(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Synchronize cache with updated real particles
     * 
     * Call this after any operation that modifies real particle properties
     * (density, pressure, smoothing length, etc.) but before neighbor search.
     * 
     * Preconditions:
     * - Cache must be initialized
     * - real_particles.size() must equal initial size (no add/remove)
     * 
     * Postconditions:
     * - Cache contains updated copies of real particles
     */
    void sync_real_particles(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Extend cache with ghost particles
     * 
     * Preconditions:
     * - Cache must contain real particles
     * - Ghost manager must be valid
     * 
     * Postconditions:
     * - Cache contains real particles followed by ghost particles
     * - Cache size equals real count + ghost count
     */
    void include_ghosts(const std::shared_ptr<GhostParticleManager<Dim>>& ghost_manager);
    
    /**
     * @brief Get read-only access to cached particles
     * 
     * Use this for neighbor search operations.
     */
    const std::vector<SPHParticle<Dim>>& get_search_particles() const {
        return cache_;
    }
    
    /**
     * @brief Get mutable access to cached particles
     * 
     * WARNING: Only use for tree building. Do not modify particle properties.
     */
    std::vector<SPHParticle<Dim>>& get_search_particles_mutable() {
        return cache_;
    }
    
    /**
     * @brief Check if cache includes ghost particles
     */
    bool has_ghosts() const {
        return has_ghosts_;
    }
    
    /**
     * @brief Get number of cached particles
     */
    size_t size() const {
        return cache_.size();
    }
    
    /**
     * @brief Check if cache is initialized
     */
    bool is_initialized() const {
        return !cache_.empty();
    }
    
    /**
     * @brief Validate cache invariants
     * 
     * Throws std::logic_error if invariants are violated.
     * Only active in debug builds.
     */
    void validate() const;
    
private:
    std::vector<SPHParticle<Dim>> cache_;
    size_t real_particle_count_ = 0;
    bool has_ghosts_ = false;
};

} // namespace sph

#include "particle_cache.tpp"

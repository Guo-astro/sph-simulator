#pragma once

#include "particle_cache.hpp"
#include "../boundaries/ghost_particle_manager.hpp"
#include "../../exception.hpp"
#include <algorithm>

namespace sph {

template<int Dim>
void ParticleCache<Dim>::initialize(const std::vector<SPHParticle<Dim>>& real_particles) {
    if (real_particles.empty()) {
        THROW_ERROR("Cannot initialize particle cache with empty particle array");
    }
    
    cache_ = real_particles;
    real_particle_count_ = real_particles.size();
    has_ghosts_ = false;
}

template<int Dim>
void ParticleCache<Dim>::sync_real_particles(const std::vector<SPHParticle<Dim>>& real_particles) {
    if (cache_.empty()) {
        THROW_ERROR("Cache not initialized. Call initialize() first.");
    }
    
    if (real_particles.size() != real_particle_count_) {
        THROW_ERROR("Real particle count mismatch. Expected: " + 
                   std::to_string(real_particle_count_) + 
                   ", Got: " + std::to_string(real_particles.size()));
    }
    
    // Update real particles in cache
    // If ghosts exist, they remain at the end
    std::copy(real_particles.begin(), real_particles.end(), cache_.begin());
}

template<int Dim>
void ParticleCache<Dim>::include_ghosts(const std::shared_ptr<GhostParticleManager<Dim>>& ghost_manager) {
    if (cache_.empty()) {
        THROW_ERROR("Cache not initialized. Call initialize() first.");
    }
    
    if (!ghost_manager || !ghost_manager->has_ghosts()) {
        // No ghosts to add
        has_ghosts_ = false;
        cache_.resize(real_particle_count_);
        return;
    }
    
    const auto& ghosts = ghost_manager->get_ghost_particles();
    
    // Resize cache to hold real + ghost particles
    const size_t total_size = real_particle_count_ + ghosts.size();
    cache_.resize(total_size);
    
    // Copy ghost particles after real particles
    // Renumber ghost IDs to match their position in combined array
    const int ghost_id_offset = static_cast<int>(real_particle_count_);
    for (size_t i = 0; i < ghosts.size(); ++i) {
        SPHParticle<Dim> ghost = ghosts[i];
        ghost.id = ghost_id_offset + static_cast<int>(i);
        cache_[real_particle_count_ + i] = ghost;
    }
    
    has_ghosts_ = true;
}

template<int Dim>
void ParticleCache<Dim>::validate() const {
#ifndef NDEBUG
    if (cache_.empty()) {
        throw std::logic_error("Particle cache is not initialized");
    }
    
    if (real_particle_count_ == 0) {
        throw std::logic_error("Real particle count is zero");
    }
    
    if (cache_.size() < real_particle_count_) {
        throw std::logic_error(
            "Cache size (" + std::to_string(cache_.size()) + 
            ") is less than real particle count (" + 
            std::to_string(real_particle_count_) + ")"
        );
    }
    
    if (has_ghosts_ && cache_.size() == real_particle_count_) {
        throw std::logic_error(
            "has_ghosts flag is true but cache only contains real particles"
        );
    }
#endif
}

} // namespace sph

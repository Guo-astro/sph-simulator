/**
 * @file spatial_tree_coordinator.tpp
 * @brief Template implementation for SpatialTreeCoordinator
 */

#pragma once

#include "core/spatial/spatial_tree_coordinator.hpp"
#include <sstream>
#include <stdexcept>

namespace sph {

template<int Dim>
SpatialTreeCoordinator<Dim>::SpatialTreeCoordinator(
    std::shared_ptr<Simulation<Dim>> sim)
    : m_sim(sim)
{
    if (!m_sim) {
        throw std::invalid_argument(
            "SpatialTreeCoordinator: simulation pointer must not be null");
    }
}

template<int Dim>
void SpatialTreeCoordinator<Dim>::rebuild_tree_for_neighbor_search()
{
    // Step 1: Synchronize search container with real + ghost particles
    synchronize_search_container();
    
    // Step 2: Clear stale linked-list pointers
    clear_linked_list_pointers();
    
    // Step 3: Validate particle IDs match indices
    validate_particle_ids();
    
    // Step 4: Rebuild spatial tree
    rebuild_spatial_tree();
}

template<int Dim>
size_t SpatialTreeCoordinator<Dim>::get_search_particle_count() const
{
    return m_sim->cached_search_particles.size();
}

template<int Dim>
bool SpatialTreeCoordinator<Dim>::is_tree_consistent() const
{
    // Tree exists and is built
    if (!m_sim->tree) {
        return false;
    }
    
    // For now, we assume consistency if tree exists
    // Future: Could expose BHTree::m_particles_ptr for validation
    return true;
}

template<int Dim>
void SpatialTreeCoordinator<Dim>::synchronize_search_container()
{
    // Get combined particle list (real + renumbered ghosts)
    // Simulation::get_all_particles_for_search() handles ID renumbering
    const auto all_particles = m_sim->get_all_particles_for_search();
    const size_t new_size = all_particles.size();
    
    // CRITICAL: Avoid reallocation to prevent tree pointer invalidation
    // If capacity is insufficient, reserve with buffer for future growth
    if (m_sim->cached_search_particles.capacity() < new_size) {
        m_sim->cached_search_particles.reserve(new_size + REALLOCATION_BUFFER);
    }
    
    // Resize and copy combined particles
    m_sim->cached_search_particles.resize(new_size);
    std::copy(all_particles.begin(), all_particles.end(), 
              m_sim->cached_search_particles.begin());
}

template<int Dim>
void SpatialTreeCoordinator<Dim>::clear_linked_list_pointers()
{
    // Tree builder (BHNode::assign) modifies particle.next for linked lists
    // These pointers become stale when particles are copied to cached_search_particles
    // Must clear before tree rebuild to prevent corruption
    for (auto& p : m_sim->cached_search_particles) {
        p.next = nullptr;
    }
}

template<int Dim>
void SpatialTreeCoordinator<Dim>::rebuild_spatial_tree()
{
#ifndef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
    if (!m_sim->tree) {
        return;
    }
    
    // Skip tree rebuild if there are no particles
    if (m_sim->cached_search_particles.empty()) {
        return;
    }
    
    // Build tree with cached_search_particles
    // Tree stores pointer to this container for validation in neighbor_search()
    m_sim->make_tree();
#endif
}

template<int Dim>
void SpatialTreeCoordinator<Dim>::validate_particle_ids() const
{
    // Verify critical invariant: particle.id == index
    // This ensures neighbor indices from tree correctly reference cached_search_particles
    for (size_t i = 0; i < m_sim->cached_search_particles.size(); ++i) {
        const int particle_id = m_sim->cached_search_particles[i].id;
        if (particle_id != static_cast<int>(i)) {
            std::ostringstream msg;
            msg << "SpatialTreeCoordinator: Particle ID mismatch detected\n"
                << "  Index in cached_search_particles: " << i << "\n"
                << "  Particle ID: " << particle_id << "\n"
                << "  Expected: ID == index\n"
                << "\n"
                << "Root cause:\n"
                << "  Ghost particles were not renumbered correctly, or\n"
                << "  particles were inserted without updating IDs.\n"
                << "\n"
                << "Solution:\n"
                << "  1. Ensure Simulation::get_all_particles_for_search() is used\n"
                << "     to get combined particle list (handles renumbering)\n"
                << "  2. Do not manually append ghost particles to cached_search_particles\n"
                << "  3. Verify ghost IDs are offset by real particle count\n"
                << "\n"
                << "Context:\n"
                << "  Real particle count: " << m_sim->particle_num << "\n"
                << "  Search particle count: " << m_sim->cached_search_particles.size();
            
            throw std::logic_error(msg.str());
        }
    }
}

} // namespace sph

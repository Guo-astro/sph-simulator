/**
 * @file ghost_particle_coordinator.tpp
 * @brief Template implementation for GhostParticleCoordinator
 */

#pragma once

#include "core/boundaries/ghost_particle_coordinator.hpp"

namespace sph {

template<int Dim>
GhostParticleCoordinator<Dim>::GhostParticleCoordinator(
    std::shared_ptr<Simulation<Dim>> sim)
    : m_sim(sim)
    , m_last_kernel_support(0.0)
{
    if (!m_sim) {
        throw std::invalid_argument(
            "GhostParticleCoordinator: simulation pointer must not be null");
    }
}

template<int Dim>
void GhostParticleCoordinator<Dim>::initialize_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles)
{
    // Early exit if ghost system is disabled or not configured
    if (!m_sim->ghost_manager || !m_sim->ghost_manager->get_config().is_valid) {
        return;
    }
    
    // Validate preconditions: smoothing lengths must be calculated
    validate_smoothing_lengths(real_particles);
    
    // Calculate kernel support from maximum smoothing length
    m_last_kernel_support = calculate_kernel_support(real_particles);
    
    // Configure ghost manager with computed support radius
    m_sim->ghost_manager->set_kernel_support_radius(m_last_kernel_support);
    
    // Generate ghost particles
    m_sim->ghost_manager->generate_ghosts(real_particles);
    
    // Log state for diagnostics
    log_ghost_state("initialize_ghosts");
}

template<int Dim>
void GhostParticleCoordinator<Dim>::update_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles)
{
    // Early exit if ghost system is disabled
    if (!m_sim->ghost_manager) {
        return;
    }
    
    // Validate smoothing lengths (may have changed during simulation)
    validate_smoothing_lengths(real_particles);
    
    // Recalculate kernel support (smoothing lengths adapt during simulation)
    m_last_kernel_support = calculate_kernel_support(real_particles);
    
    // Update ghost manager configuration
    m_sim->ghost_manager->set_kernel_support_radius(m_last_kernel_support);
    
    // Update ghost particle positions and properties
    m_sim->ghost_manager->update_ghosts(real_particles);
}

template<int Dim>
void GhostParticleCoordinator<Dim>::update_ghost_properties(
    const std::vector<SPHParticle<Dim>>& real_particles)
{
    // Early exit if ghost system is disabled
    if (!m_sim->ghost_manager) {
        return;
    }
    
    // Update calculated properties (density, pressure, energy, velocity)
    // from source real particles after they've been updated
    m_sim->ghost_manager->update_ghost_calculated_properties(real_particles);
    
    // Sync the updated ghosts back to the simulation's cached_search_particles
    // Ghosts are stored at indices [particle_num, total_count) in cached_search_particles
    const auto& ghosts = m_sim->ghost_manager->get_ghost_particles();
    for (size_t i = 0; i < ghosts.size(); ++i) {
        m_sim->cached_search_particles[m_sim->particle_num + i] = ghosts[i];
    }
}

template<int Dim>
bool GhostParticleCoordinator<Dim>::has_ghosts() const
{
    if (!m_sim->ghost_manager) {
        return false;
    }
    
    return m_sim->ghost_manager->get_ghost_count() > 0;
}

template<int Dim>
size_t GhostParticleCoordinator<Dim>::ghost_count() const
{
    if (!m_sim->ghost_manager) {
        return 0;
    }
    
    return m_sim->ghost_manager->get_ghost_count();
}

template<int Dim>
real GhostParticleCoordinator<Dim>::get_kernel_support_radius() const
{
    return m_last_kernel_support;
}

template<int Dim>
real GhostParticleCoordinator<Dim>::calculate_kernel_support(
    const std::vector<SPHParticle<Dim>>& particles) const
{
    if (particles.empty()) {
        return 0.0;
    }
    
    // Find maximum smoothing length
    real max_sml = 0.0;
    for (const auto& p : particles) {
        if (p.sml > max_sml) {
            max_sml = p.sml;
        }
    }
    
    // For cubic spline kernel, compact support is 2h
    return CUBIC_SPLINE_SUPPORT_FACTOR * max_sml;
}

template<int Dim>
void GhostParticleCoordinator<Dim>::validate_smoothing_lengths(
    const std::vector<SPHParticle<Dim>>& particles) const
{
    for (size_t i = 0; i < particles.size(); ++i) {
        if (!std::isfinite(particles[i].sml) || particles[i].sml <= 0.0) {
            std::ostringstream msg;
            msg << "GhostParticleCoordinator: Invalid smoothing length detected\n"
                << "  Particle index: " << i << "\n"
                << "  Particle ID: " << particles[i].id << "\n"
                << "  Smoothing length (sml): " << particles[i].sml << "\n"
                << "  Expected: finite value > 0\n"
                << "\n"
                << "Root cause:\n"
                << "  Ghost generation requires valid smoothing lengths.\n"
                << "  This indicates pre-interaction (smoothing length calculation)\n"
                << "  was not called before ghost initialization, or smoothing length\n"
                << "  calculation failed for this particle.\n"
                << "\n"
                << "Solution:\n"
                << "  1. Ensure pre-interaction->calculation() is called BEFORE\n"
                << "     GhostParticleCoordinator::initialize_ghosts()\n"
                << "  2. Check particle density and neighbor count are valid\n"
                << "  3. Verify initial conditions set reasonable particle spacing";
            
            throw std::logic_error(msg.str());
        }
    }
}

template<int Dim>
void GhostParticleCoordinator<Dim>::log_ghost_state(const std::string& context) const
{
    // Debug logging disabled for production use
    // Uncomment for debugging ghost particle issues:
    // if (!m_sim->ghost_manager) {
    //     return;
    // }
    // WRITE_LOG << "[GhostParticleCoordinator::" << context << "]";
    // WRITE_LOG << "  Kernel support radius: " << m_last_kernel_support;
    // WRITE_LOG << "  Ghost count: " << m_sim->ghost_manager->get_ghost_count();
}

} // namespace sph

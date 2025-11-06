#pragma once

#include "simulation.hpp"
#include "../../parameters.hpp"
#include "../../exception.hpp"
#include "../boundaries/periodic.hpp"
#include "../spatial/bhtree.hpp"
#include "../kernels/cubic_spline.hpp"
#include "../kernels/wendland_kernel.hpp"
#include "../boundaries/ghost_particle_manager.hpp"

namespace sph {

template<int Dim>
Simulation<Dim>::Simulation(std::shared_ptr<SPHParameters> param) {
    if (param->kernel == KernelType::CUBIC_SPLINE) {
        kernel = std::make_shared<Spline::Cubic<Dim>>();
    } else if (param->kernel == KernelType::WENDLAND) {
        if constexpr (Dim >= 2) {
            kernel = std::make_shared<Wendland::C4Kernel<Dim>>();
        } else {
            THROW_ERROR("Wendland kernel not available for 1D");
        }
    } else {
        THROW_ERROR("kernel is unknown.");
    }

    periodic = std::make_shared<Periodic<Dim>>();
    periodic->initialize(param);
    
    // Initialize ghost particle manager
    ghost_manager = std::make_shared<GhostParticleManager<Dim>>();

    tree = std::make_shared<BHTree<Dim>>();
    tree->initialize(param);

    time = param->time.start;
    dt = 0.0;
}

template<int Dim>
void Simulation<Dim>::update_time() {
    time += dt;
}

template<int Dim>
void Simulation<Dim>::make_tree() {
    // Build tree with cached search particles (already populated by solver)
    // CRITICAL: Cached vector must remain valid throughout neighbor search
    const int total_count = static_cast<int>(cached_search_particles.size());
    tree->make(cached_search_particles, total_count);
}

template<int Dim>
void Simulation<Dim>::add_scalar_array(const std::vector<std::string>& names) {
    const int num = particle_num;
    for (const auto& name : names) {
        additional_scalar_array[name].resize(num);
    }
}

template<int Dim>
void Simulation<Dim>::add_vector_array(const std::vector<std::string>& names) {
    const int num = particle_num;
    for (const auto& name : names) {
        additional_vector_array[name].resize(num);
    }
}

template<int Dim>
std::vector<real>& Simulation<Dim>::get_scalar_array(const std::string& name) {
    auto it = additional_scalar_array.find(name);
    if (it != additional_scalar_array.end()) {
        return it->second;
    } else {
        THROW_ERROR("additional_scalar_array does not have ", name);
    }
}

template<int Dim>
std::vector<Vector<Dim>>& Simulation<Dim>::get_vector_array(const std::string& name) {
    auto it = additional_vector_array.find(name);
    if (it != additional_vector_array.end()) {
        return it->second;
    } else {
        THROW_ERROR("additional_vector_array does not have ", name);
    }
}

template<int Dim>
std::vector<SPHParticle<Dim>> Simulation<Dim>::get_all_particles_for_search() const {
    if (ghost_manager && ghost_manager->has_ghosts()) {
        // Combine real and ghost particles
        std::vector<SPHParticle<Dim>> all_particles = particles;
        const auto& ghosts = ghost_manager->get_ghost_particles();
        
        // CRITICAL: Reserve space to avoid reallocation during insertion
        all_particles.reserve(particles.size() + ghosts.size());
        
        // Insert ghost particles and renumber their IDs to match their position
        const int ghost_id_offset = static_cast<int>(particles.size());
        for (size_t i = 0; i < ghosts.size(); ++i) {
            SPHParticle<Dim> ghost = ghosts[i];
            // CRITICAL FIX: Ghost particle ID must match its index in the combined vector
            ghost.id = ghost_id_offset + static_cast<int>(i);
            all_particles.push_back(ghost);
        }
        
        return all_particles;
    } else {
        // No ghosts, return real particles only
        return particles;
    }
}

template<int Dim>
int Simulation<Dim>::get_total_particle_count() const {
    int total = particle_num;
    if (ghost_manager && ghost_manager->has_ghosts()) {
        total += ghost_manager->get_ghost_count();
    }
    return total;
}

// Explicit template instantiations
template class Simulation<1>;
template class Simulation<2>;
template class Simulation<3>;

} // namespace sph

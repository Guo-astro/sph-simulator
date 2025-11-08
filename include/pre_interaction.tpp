#include <algorithm>
#include <cmath>

#include "parameters.hpp"
#include "pre_interaction.hpp"
#include "core/simulation/simulation.hpp"
#include "core/boundaries/periodic.hpp"
#include "openmp.hpp"
#include "core/kernels/kernel_function.hpp"
#include "exception.hpp"
#include "core/spatial/bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
#include "exhaustive_search.hpp"
#endif

namespace sph
{

template<int Dim>
void PreInteraction<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    m_use_time_dependent_av = param->get_av().use_time_dependent_av;
    if(m_use_time_dependent_av) {
        m_alpha_max = param->get_av().alpha_max;
        m_alpha_min = param->get_av().alpha_min;
        m_epsilon = param->get_av().epsilon;
    }
    m_use_balsara_switch = param->get_av().use_balsara_switch;
    m_adiabatic_index = param->get_physics().gamma;
    m_neighbor_number = param->get_physics().neighbor_number;
    m_iteration = param->get_iterative_sml();
    if(m_iteration) {
        m_kernel_ratio = 1.2;
    } else {
        m_kernel_ratio = 1.0;
    }
    m_first = true;
    
    // Extract smoothing length minimum enforcement configuration
    const auto& sml_config = param->get_smoothing_length();
    m_sml_policy = sml_config.policy;
    m_sml_h_min_constant = sml_config.h_min_constant;
    m_sml_expected_max_density = sml_config.expected_max_density;
    m_sml_h_min_coefficient = sml_config.h_min_coefficient;
}

template<int Dim>
void PreInteraction<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    if(m_first) {
        initial_smoothing(sim);
        m_first = false;
    }

    auto & particles = sim->particles;
    auto * periodic = sim->periodic.get();
    const int num = sim->particle_num;
    auto * kernel = sim->kernel.get();
    const real dt = sim->dt;
    auto * tree = sim->tree.get();

    // Use cached combined particle list (built when tree was created)
    auto & search_particles = sim->cached_search_particles;
    const int search_size = static_cast<int>(search_particles.size());

    omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {  // Only iterate over real particles
        auto & p_i = particles[i];
        std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

        // guess smoothing length
        constexpr real A = Dim == 1 ? 2.0 :
                           Dim == 2 ? M_PI :
                                      4.0 * M_PI / 3.0;
        
        // CRITICAL FIX: Guard against zero or very small density (same as initial_smoothing)
        constexpr real dens_min = 1.0e-20;
        const real dens_safe = std::max(p_i.dens, dens_min);
        
        p_i.sml = std::pow(m_neighbor_number * p_i.mass / (dens_safe * A), 1.0 / Dim) * m_kernel_ratio;
        
        // CRITICAL FIX: Ensure smoothing length stays finite and reasonable
        if (!std::isfinite(p_i.sml) || p_i.sml <= 0.0 || p_i.sml > 1.0e10) {
#pragma omp critical
            {
                WRITE_LOG << "WARNING: Particle id " << p_i.id << " has invalid sml=" << p_i.sml << " at timestep";
                WRITE_LOG << "  dens=" << p_i.dens << ", mass=" << p_i.mass << ", resetting to safe value";
            }
            // Use previous value or reasonable default
            p_i.sml = std::pow(p_i.mass / dens_safe, 1.0 / Dim) * m_kernel_ratio;
        }
        
        // Create search config (declarative)
        const auto search_config = NeighborSearchConfig::create(m_neighbor_number, /*is_ij=*/false);
        
        // neighbor search (searches in real + ghost particles) - REFACTORED
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
        std::vector<int> neighbor_list(search_config.max_neighbors);
        const int search_num = static_cast<int>(search_particles.size());
        const int n_neighbor_tmp = exhaustive_search(p_i, p_i.sml, search_particles, search_num, 
                                                     neighbor_list, search_config.max_neighbors, periodic, false);
        auto result = NeighborSearchResult{
            .neighbor_indices = std::vector<int>(neighbor_list.begin(), neighbor_list.begin() + n_neighbor_tmp),
            .is_truncated = false,
            .total_candidates_found = n_neighbor_tmp
        };
#else
        // REFACTORED: Declarative neighbor search
        auto result = tree->find_neighbors(p_i, search_config);
#endif
        
        // Bounds checking now handled by find_neighbors() validation
        // REFACTORED: Use result.neighbor_indices
        for (size_t n = 0; n < result.neighbor_indices.size(); ++n) {
            const int j = result.neighbor_indices[n];
            if (j < 0 || j >= search_size) {
#pragma omp critical
                {
                    WRITE_LOG << "ERROR: Particle " << i << " has neighbor index " << j 
                             << " which is out of bounds [0, " << search_size << ")";
                }
            }
        }
        // smoothing length
        if(m_iteration) {
            p_i.sml = newton_raphson(p_i, search_particles, result.neighbor_indices, 
                                    static_cast<int>(result.neighbor_indices.size()), periodic, kernel);
        }

        // density etc.
        real dens_i = 0.0;
        real dh_dens_i = 0.0;
        real v_sig_max = p_i.sound * 2.0;
        const Vector<Dim> & pos_i = p_i.pos;
        int n_neighbor = 0;
        for(int n = 0; n < static_cast<int>(result.neighbor_indices.size()); ++n) {
            int const j = result.neighbor_indices[n];
            auto & p_j = search_particles[j];  // Access from combined list
            const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= p_i.sml) {
                break;
            }

            ++n_neighbor;
            dens_i += p_j.mass * kernel->w(r, p_i.sml);
            dh_dens_i += p_j.mass * kernel->dhw(r, p_i.sml);

            if(i != j) {
                const real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / r;
                if(v_sig > v_sig_max) {
                    v_sig_max = v_sig;
                }
            }
        }

        // CRITICAL FIX: Prevent zero density which causes sml->inf in next iteration
        {
            constexpr real dens_min = 1.0e-20;
            if (dens_i < dens_min || !std::isfinite(dens_i)) {
#pragma omp critical
                {
                    WRITE_LOG << "WARNING: Particle id " << p_i.id << " has invalid computed density=" << dens_i;
                    WRITE_LOG << "  n_neighbor=" << n_neighbor << ", sml=" << p_i.sml << ", mass=" << p_i.mass;
                    WRITE_LOG << "  Using fallback: self-density = mass * W(0, sml)";
                }
                // Fallback: use self-density (particle's own contribution)
                auto* kernel = sim->kernel.get();
                dens_i = p_i.mass * kernel->w(0.0, p_i.sml);
                if (dens_i < dens_min) {
                    dens_i = dens_min;  // Ultimate fallback
                }
            }
        }

        p_i.dens = dens_i;
        p_i.pres = (m_adiabatic_index - 1.0) * dens_i * p_i.ene;
        p_i.gradh = 1.0 / (1.0 + p_i.sml / (Dim * dens_i) * dh_dens_i);
        p_i.neighbor = n_neighbor;

        const real h_per_v_sig_i = p_i.sml / v_sig_max;
        if(h_per_v_sig.get() > h_per_v_sig_i) {
            h_per_v_sig.get() = h_per_v_sig_i;
        }

        // Artificial viscosity
        if(m_use_balsara_switch && Dim != 1) {
            // balsara switch
            real div_v = 0.0;
            real rot_v = 0.0;
            for(int n = 0; n < n_neighbor; ++n) {
                int const j = neighbor_list[n];
                auto & p_j = search_particles[j];  // Access from combined list
                const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = abs(r_ij);
                const Vector<Dim> dw = kernel->dw(r_ij, r, p_i.sml);
                const Vector<Dim> v_ij = p_i.vel - p_j.vel;
                div_v -= p_j.mass * inner_product(v_ij, dw);
                if constexpr (Dim == 2) {
                    rot_v += vector_product(v_ij, dw) * p_j.mass;
                } else if constexpr (Dim == 3) {
                    rot_v += abs(cross_product(v_ij, dw)) * p_j.mass;
                }
            }
            div_v /= p_i.dens;
            rot_v /= p_i.dens;
            p_i.balsara = std::abs(div_v) / (std::abs(div_v) + std::abs(rot_v) + 1e-4 * p_i.sound / p_i.sml);

            // time dependent alpha
            if(m_use_time_dependent_av) {
                const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                const real dalpha = (-(p_i.alpha - m_alpha_min) * tau_inv + std::max(-div_v, (real)0.0) * (m_alpha_max - p_i.alpha)) * dt;
                p_i.alpha += dalpha;
            }
        } else if(m_use_time_dependent_av) {
            real div_v = 0.0;
            for(int n = 0; n < n_neighbor; ++n) {
                int const j = neighbor_list[n];
                auto & p_j = search_particles[j];  // Access from combined list
                const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = abs(r_ij);
                const Vector<Dim> dw = kernel->dw(r_ij, r, p_i.sml);
                const Vector<Dim> v_ij = p_i.vel - p_j.vel;
                div_v -= p_j.mass * inner_product(v_ij, dw);
            }
            div_v /= p_i.dens;
            const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
            const real s_i = std::max(-div_v, (real)0.0);
            p_i.alpha = (p_i.alpha + dt * tau_inv * m_alpha_min + s_i * dt * m_alpha_max) / (1.0 + dt * tau_inv + s_i * dt);
        }
    }

    sim->h_per_v_sig = h_per_v_sig.min();

#ifndef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG_ONLY_FOR_DEBUG
    tree->set_kernel();
#endif
}

template<int Dim>
void PreInteraction<Dim>::initial_smoothing(std::shared_ptr<Simulation<Dim>> sim)
{
    auto & particles = sim->particles;
    auto * periodic = sim->periodic.get();
    const int num = sim->particle_num;
    auto * kernel = sim->kernel.get();
    auto * tree = sim->tree.get();

    // Use cached combined particle list (built when tree was created)
    auto & search_particles = sim->cached_search_particles;

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {  // Only iterate over real particles
        auto & p_i = particles[i];
        const Vector<Dim> & pos_i = p_i.pos;
        std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

        // guess smoothing length
        constexpr real A = Dim == 1 ? 2.0 :
                           Dim == 2 ? M_PI :
                                      4.0 * M_PI / 3.0;
        
        // CRITICAL FIX: Guard against zero or very small density
        constexpr real dens_min = 1.0e-20;
        const real dens_safe = std::max(p_i.dens, dens_min);
        
        p_i.sml = std::pow(m_neighbor_number * p_i.mass / (dens_safe * A), 1.0 / Dim);
        
        // CRITICAL FIX: Ensure smoothing length is finite and reasonable
        if (!std::isfinite(p_i.sml) || p_i.sml <= 0.0 || p_i.sml > 1.0e10) {
#pragma omp critical
            {
                WRITE_LOG << "WARNING: Particle id " << p_i.id << " has invalid initial sml=" << p_i.sml;
                WRITE_LOG << "  dens=" << p_i.dens << ", mass=" << p_i.mass;
            }
            // Use a reasonable default based on typical particle spacing
            p_i.sml = std::pow(p_i.mass / dens_safe, 1.0 / Dim);
        }
        
        // neighbor search (searches in real + ghost particles)
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
        const int search_num = static_cast<int>(search_particles.size());
        int const n_neighbor = exhaustive_search(p_i, p_i.sml, search_particles, search_num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, false);
        auto result = NeighborSearchResult{neighbor_list, false, n_neighbor};
#else
        const auto search_config = NeighborSearchConfig::create(m_neighbor_number, false);
        auto result = tree->find_neighbors(p_i, search_config);
#endif

        // density
        real dens_i = 0.0;
        for(int n = 0; n < static_cast<int>(result.neighbor_indices.size()); ++n) {
            int const j = result.neighbor_indices[n];
            auto & p_j = search_particles[j];  // Access from combined list
            const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= p_i.sml) {
                break;
            }

            dens_i += p_j.mass * kernel->w(r, p_i.sml);
        }

        // CRITICAL FIX: Prevent zero density (same as main loop above)
        {
            constexpr real dens_min = 1.0e-20;
            if (dens_i < dens_min || !std::isfinite(dens_i)) {
#pragma omp critical
                {
                    WRITE_LOG << "WARNING: Particle id " << p_i.id << " has invalid initial density=" << dens_i;
                    WRITE_LOG << "  Using self-density fallback";
                }
                dens_i = p_i.mass * kernel->w(0.0, p_i.sml);
                if (dens_i < dens_min) {
                    dens_i = dens_min;
                }
            }
        }

        p_i.dens = dens_i;
    }
}

template<int Dim>
real powh_(const real h) {
    if constexpr(Dim == 1) {
        return 1;
    } else if constexpr(Dim == 2) {
        return h;
    } else if constexpr(Dim == 3) {
        return h * h;
    }
}

template<int Dim>
real PreInteraction<Dim>::newton_raphson(
    const SPHParticle<Dim> & p_i,
    const std::vector<SPHParticle<Dim>> & particles,
    const std::vector<int> & neighbor_list,
    const int n_neighbor,
    const Periodic<Dim> * periodic,
    const KernelFunction<Dim> * kernel
)
{
    real h_i = p_i.sml / m_kernel_ratio;
    constexpr real A = Dim == 1 ? 2.0 :
                       Dim == 2 ? M_PI :
                                  4.0 * M_PI / 3.0;
    const real b = p_i.mass * m_neighbor_number / A;

    // f = rho h^d - b
    // f' = drho/dh h^d + d rho h^{d-1}

    constexpr real epsilon = 1e-4;
    constexpr int max_iter = 10;
    const auto & r_i = p_i.pos;
    for(int i = 0; i < max_iter; ++i) {
        const real h_b = h_i;

        real dens = 0.0;
        real ddens = 0.0;
        for(int n = 0; n < n_neighbor; ++n) {
            int const j = neighbor_list[n];
            
            // Safety check for array bounds
            if (j < 0 || j >= static_cast<int>(particles.size())) {
#pragma omp critical
                {
                    WRITE_LOG << "ERROR: newton_raphson accessing invalid index j=" << j 
                              << ", particles.size()=" << particles.size() 
                              << ", n=" << n << ", n_neighbor=" << n_neighbor;
                }
                continue;  // Skip this neighbor
            }
            
            auto & p_j = particles[j];
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= h_i) {
                break;
            }

            dens += p_j.mass * kernel->w(r, h_i);
            ddens += p_j.mass * kernel->dhw(r, h_i);
        }

        const real f = dens * powh<Dim>(h_i) - b;
        const real df = ddens * powh<Dim>(h_i) + Dim * dens * powh_<Dim>(h_i);

        // CRITICAL FIX: Guard against division by zero or very small derivative
        // This prevents h_i from becoming infinite when df -> 0
        constexpr real df_min = 1.0e-30;
        if (std::abs(df) < df_min) {
#pragma omp critical
            {
                WRITE_LOG << "WARNING: Particle id " << p_i.id << " has df close to zero: df=" << df;
                WRITE_LOG << "  dens=" << dens << ", ddens=" << ddens << ", h_i=" << h_i;
                WRITE_LOG << "  Returning initial guess to avoid inf/nan";
            }
            return p_i.sml / m_kernel_ratio;  // Return safe initial guess
        }

        const real dh = f / df;
        
        // CRITICAL FIX: Limit the step size to prevent runaway
        // Max change per iteration: 20% of current value
        const real max_dh = 0.2 * h_i;
        const real dh_limited = std::max(-max_dh, std::min(max_dh, dh));
        
        h_i -= dh_limited;
        
        // CRITICAL FIX: Ensure h_i stays positive and finite
        if (!std::isfinite(h_i) || h_i <= 0.0) {
#pragma omp critical
            {
                WRITE_LOG << "ERROR: Particle id " << p_i.id << " has invalid h_i=" << h_i;
                WRITE_LOG << "  Returning initial guess";
            }
            return p_i.sml / m_kernel_ratio;  // Return safe initial guess
        }

        if(std::abs(h_i - h_b) < (h_i + h_b) * epsilon) {
            // Apply minimum smoothing length policy if configured
            real h_result = h_i;
            
            switch (m_sml_policy) {
                case SPHParameters::SmoothingLengthPolicy::NO_MIN:
                    // No enforcement - allow natural collapse
                    h_result = h_i;
                    break;
                    
                case SPHParameters::SmoothingLengthPolicy::CONSTANT_MIN:
                    // Enforce constant minimum
                    h_result = std::max(h_i, m_sml_h_min_constant);
                    break;
                    
                case SPHParameters::SmoothingLengthPolicy::PHYSICS_BASED: {
                    // Physics-based minimum: h_min = coeff * (m/rho_max)^(1/dim)
                    // This prevents h from collapsing below the physical resolution
                    // set by particle mass and maximum expected density
                    const real d_min = std::pow(p_i.mass / m_sml_expected_max_density, 1.0 / Dim);
                    const real h_min_physical = m_sml_h_min_coefficient * d_min;
                    h_result = std::max(h_i, h_min_physical);
                    break;
                }
            }
            
            return h_result;
        }
    }

#pragma omp critical
    {
        WRITE_LOG << "Particle id " << p_i.id << " is not convergence";
        WRITE_LOG << "  Position: " << p_i.pos[0] << ", sml: " << p_i.sml 
                  << ", dens: " << p_i.dens << ", mass: " << p_i.mass;
    }

    // Apply minimum policy even if not converged
    real h_result = p_i.sml / m_kernel_ratio;
    
    switch (m_sml_policy) {
        case SPHParameters::SmoothingLengthPolicy::NO_MIN:
            h_result = p_i.sml / m_kernel_ratio;
            break;
            
        case SPHParameters::SmoothingLengthPolicy::CONSTANT_MIN:
            h_result = std::max(p_i.sml / m_kernel_ratio, m_sml_h_min_constant);
            break;
            
        case SPHParameters::SmoothingLengthPolicy::PHYSICS_BASED: {
            const real d_min = std::pow(p_i.mass / m_sml_expected_max_density, 1.0 / Dim);
            const real h_min_physical = m_sml_h_min_coefficient * d_min;
            h_result = std::max(p_i.sml / m_kernel_ratio, h_min_physical);
            break;
        }
    }
    
    return h_result;
}

}

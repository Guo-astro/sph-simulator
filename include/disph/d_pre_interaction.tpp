#include <algorithm>

#include "disph/d_pre_interaction.hpp"
#include "parameters.hpp"
#include "core/simulation.hpp"
#include "core/periodic.hpp"
#include "openmp.hpp"
#include "core/kernel_function.hpp"
#include "exception.hpp"
#include "core/bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "exhaustive_search.hpp"
#endif

namespace sph
{
namespace disph
{

template<int Dim>
void PreInteraction<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    if(this->m_first) {
        this->initial_smoothing(sim);
        this->m_first = false;
    }

    auto & particles = sim->particles;
    auto * periodic = sim->periodic.get();
    const int num = sim->particle_num;
    auto * kernel = sim->kernel.get();
    const real dt = sim->dt;
    auto * tree = sim->tree.get();

    omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        std::vector<int> neighbor_list(this->m_neighbor_number * neighbor_list_size);

        // guess smoothing length
        constexpr real A = Dim == 1 ? 2.0 :
                           Dim == 2 ? M_PI :
                                      4.0 * M_PI / 3.0;
        p_i.sml = std::pow(this->m_neighbor_number * p_i.mass / (p_i.dens * A), 1.0 / Dim) * this->m_kernel_ratio;
        
        // neighbor search
#ifdef EXHAUSTIVE_SEARCH
        const int n_neighbor_tmp = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, this->m_neighbor_number * neighbor_list_size, periodic, false);
#else
        const int n_neighbor_tmp = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif
        // smoothing length
        if(this->m_iteration) {
            p_i.sml = newton_raphson(p_i, particles, neighbor_list, n_neighbor_tmp, periodic, kernel);
        }

        // density etc.
        real dens_i = 0.0;
        real pres_i = 0.0;
        real dh_pres_i = 0.0;
        real n_i = 0.0;
        real dh_n_i = 0.0;
        real v_sig_max = p_i.sound * 2.0;
        const Vector<Dim> & pos_i = p_i.pos;
        int n_neighbor = 0;
        for(int n = 0; n < n_neighbor_tmp; ++n) {
            int const j = neighbor_list[n];
            auto & p_j = particles[j];
            const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= p_i.sml) {
                break;
            }

            ++n_neighbor;
            const real w_ij = kernel->w(r, p_i.sml);
            const real dhw_ij = kernel->dhw(r, p_i.sml);
            dens_i += p_j.mass * w_ij;
            n_i += w_ij;
            pres_i += p_j.mass * p_j.ene * w_ij;
            dh_pres_i += p_j.mass * p_j.ene * dhw_ij;
            dh_n_i += dhw_ij;

            if(i != j) {
                const real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / r;
                if(v_sig > v_sig_max) {
                    v_sig_max = v_sig;
                }
            }
        }

        p_i.dens = dens_i;
        p_i.pres = (this->m_adiabatic_index - 1.0) * pres_i;
        // f_ij = 1 - p_i.gradh / (p_j.mass * p_j.ene)
        p_i.gradh = p_i.sml / (Dim * n_i) * dh_pres_i / (1.0 + p_i.sml / (Dim * n_i) * dh_n_i);
        p_i.neighbor = n_neighbor;

        const real h_per_v_sig_i = p_i.sml / v_sig_max;
        if(h_per_v_sig.get() > h_per_v_sig_i) {
            h_per_v_sig.get() = h_per_v_sig_i;
        }

        // Artificial viscosity
        if(this->m_use_balsara_switch && Dim != 1) {
            // balsara switch
            real div_v = 0.0;
            real rot_v = 0.0;
            for(int n = 0; n < n_neighbor; ++n) {
                int const j = neighbor_list[n];
                auto & p_j = particles[j];
                const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = abs(r_ij);
                const Vector<Dim> dw = kernel->dw(r_ij, r, p_i.sml);
                const Vector<Dim> v_ij = p_i.vel - p_j.vel;
                div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
                if constexpr (Dim == 2) {
                    rot_v += vector_product(v_ij, dw) * (p_j.mass * p_j.ene);
                } else if constexpr (Dim == 3) {
                    rot_v += abs(cross_product(v_ij, dw)) * (p_j.mass * p_j.ene);
                }
            }
            const real p_inv = (this->m_adiabatic_index - 1.0) / p_i.pres;
            div_v *= p_inv;
            rot_v *= p_inv;
            p_i.balsara = std::abs(div_v) / (std::abs(div_v) + std::abs(rot_v) + 1e-4 * p_i.sound / p_i.sml);

            // time dependent alpha
            if(this->m_use_time_dependent_av) {
                const real tau_inv = this->m_epsilon * p_i.sound / p_i.sml;
                const real dalpha = (-(p_i.alpha - this->m_alpha_min) * tau_inv + std::max(-div_v, (real)0.0) * (this->m_alpha_max - p_i.alpha)) * dt;
                p_i.alpha += dalpha;
            }
        } else if(this->m_use_time_dependent_av) {
            real div_v = 0.0;
            for(int n = 0; n < n_neighbor; ++n) {
                int const j = neighbor_list[n];
                auto & p_j = particles[j];
                const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = abs(r_ij);
                const Vector<Dim> dw = kernel->dw(r_ij, r, p_i.sml);
                const Vector<Dim> v_ij = p_i.vel - p_j.vel;
                div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
            }
            const real p_inv = (this->m_adiabatic_index - 1.0) / p_i.pres;
            div_v *= p_inv;
            const real tau_inv = this->m_epsilon * p_i.sound / p_i.sml;
            const real s_i = std::max(-div_v, (real)0.0);
            p_i.alpha = (p_i.alpha + dt * tau_inv * this->m_alpha_min + s_i * dt * this->m_alpha_max) / (1.0 + dt * tau_inv + s_i * dt);
        }
    }

    sim->h_per_v_sig = h_per_v_sig.min();

#ifndef EXHAUSTIVE_SEARCH
    tree->set_kernel();
#endif
}

template<int Dim>
real dpowh_(const real h) {
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
    real h_i = p_i.sml / this->m_kernel_ratio;
    constexpr real A = Dim == 1 ? 2.0 :
                       Dim == 2 ? M_PI :
                                  4.0 * M_PI / 3.0;
    const real b = p_i.mass * this->m_neighbor_number / A;

    // f = n h^d - b
    // f' = dn/dh h^d + d n h^{d-1}

    constexpr real epsilon = 1e-4;
    constexpr int max_iter = 10;
    const auto & r_i = p_i.pos;
    for(int i = 0; i < max_iter; ++i) {
        const real h_b = h_i;

        real dens = 0.0;
        real ddens = 0.0;
        for(int n = 0; n < n_neighbor; ++n) {
            int const j = neighbor_list[n];
            auto & p_j = particles[j];
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= h_i) {
                break;
            }

            dens += kernel->w(r, h_i);
            ddens += kernel->dhw(r, h_i);
        }

        const real f = dens * powh<Dim>(h_i) - b;
        const real df = ddens * powh<Dim>(h_i) + Dim * dens * dpowh_<Dim>(h_i);

        h_i -= f / df;

        if(std::abs(h_i - h_b) < (h_i + h_b) * epsilon) {
            return h_i;
        }
    }

#pragma omp critical
    {
        WRITE_LOG << "Particle id " << p_i.id << " is not convergence";
    }

    return p_i.sml / this->m_kernel_ratio;
}

}
}

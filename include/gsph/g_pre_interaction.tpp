#include <algorithm>

#include "parameters.hpp"
#include "gsph/g_pre_interaction.hpp"
#include "core/simulation.hpp"
#include "core/periodic.hpp"
#include "openmp.hpp"
#include "core/kernel_function.hpp"
#include "exception.hpp"
#include "core/bhtree.hpp"
#include "utilities/constants.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "exhaustive_search.hpp"
#endif

namespace sph
{
namespace gsph
{

template<int Dim>
void PreInteraction<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    sph::PreInteraction<Dim>::initialize(param);
    this->m_is_2nd_order = param->gsph.is_2nd_order;
}

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
    auto * tree = sim->tree.get();

    omp_real h_per_v_sig(std::numeric_limits<real>::max());

    // for MUSCL
    auto & grad_d = sim->get_vector_array("grad_density");
    auto & grad_p = sim->get_vector_array("grad_pressure");
    Vector<Dim> * grad_v[Dim];
    grad_v[0] = sim->get_vector_array("grad_velocity_0").data();
    if constexpr (Dim >= 2) {
        grad_v[1] = sim->get_vector_array("grad_velocity_1").data();
    }
    if constexpr (Dim == 3) {
        grad_v[2] = sim->get_vector_array("grad_velocity_2").data();
    }

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        std::vector<int> neighbor_list(this->m_neighbor_number * neighbor_list_size);

        // guess smoothing length
        constexpr real A = Dim == 1 ? utilities::constants::UNIT_SPHERE_VOLUME_1D :
                           Dim == 2 ? utilities::constants::UNIT_SPHERE_VOLUME_2D :
                                      utilities::constants::UNIT_SPHERE_VOLUME_3D;
        p_i.sml = std::pow(this->m_neighbor_number * p_i.mass / (p_i.dens * A), utilities::constants::ONE / Dim) * this->m_kernel_ratio;
        
        // neighbor search
#ifdef EXHAUSTIVE_SEARCH
        const int n_neighbor_tmp = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, this->m_neighbor_number * neighbor_list_size, periodic, false);
#else
        const int n_neighbor_tmp = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif
        // smoothing length
        if(this->m_iteration) {
            p_i.sml = this->newton_raphson(p_i, particles, neighbor_list, n_neighbor_tmp, periodic, kernel);
        }

        // density etc.
        real dens_i = utilities::constants::ZERO;
        real v_sig_max = p_i.sound * utilities::constants::TWO;
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
            dens_i += p_j.mass * kernel->w(r, p_i.sml);

            if(i != j) {
                const real v_sig = p_i.sound + p_j.sound - utilities::constants::SIGNAL_VELOCITY_COEFF * inner_product(r_ij, p_i.vel - p_j.vel) / r;
                if(v_sig > v_sig_max) {
                    v_sig_max = v_sig;
                }
            }
        }

        p_i.dens = dens_i;
        p_i.pres = (this->m_adiabatic_index - utilities::constants::ONE) * dens_i * p_i.ene;
        p_i.neighbor = n_neighbor;

        const real h_per_v_sig_i = p_i.sml / v_sig_max;
        if(h_per_v_sig.get() > h_per_v_sig_i) {
            h_per_v_sig.get() = h_per_v_sig_i;
        }

        // MUSCL法のための勾配計算
        if(!this->m_is_2nd_order) {
            continue;
        }

        Vector<Dim> dd, du; // dP = (gamma - 1) * (rho * du + drho * u)
        Vector<Dim> dv[DIM];
        for(int n = 0; n < n_neighbor; ++n) {
            int const j = neighbor_list[n];
            auto & p_j = particles[j];
            const Vector<Dim> r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
            const real r = abs(r_ij);
            const Vector<Dim> dw_ij = kernel->dw(r_ij, r, p_i.sml);
            dd += dw_ij * p_j.mass;
            du += dw_ij * (p_j.mass * (p_j.ene - p_i.ene));
            for(int k = 0; k < DIM; ++k) {
                dv[k] += dw_ij * (p_j.mass * (p_j.vel[k] - p_i.vel[k]));
            }
        }
        grad_d[i] = dd;
        grad_p[i] = (dd * p_i.ene + du) * (this->m_adiabatic_index - utilities::constants::ONE);
        const real rho_inv = utilities::constants::ONE / p_i.dens;
        for(int k = 0; k < DIM; ++k) {
            grad_v[k][i] = dv[k] * rho_inv;
        }
    }

    sim->h_per_v_sig = h_per_v_sig.min();

#ifndef EXHAUSTIVE_SEARCH
    tree->set_kernel();
#endif
}

}
}

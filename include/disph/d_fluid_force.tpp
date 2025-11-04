#include "defines.hpp"
#include "disph/d_fluid_force.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"
#include "core/simulation.hpp"
#include "core/bhtree.hpp"
#include "core/kernel_function.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "exhaustive_search.hpp"
#endif

namespace sph
{
namespace disph
{

template<int Dim>
void FluidForce<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    sph::FluidForce<Dim>::initialize(param);
    this->m_adiabatic_index = param->physics.gamma;
}

// Hopkins (2013)
// pressure-energy formulation
template<int Dim>
void FluidForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    auto & particles = sim->particles;
    auto * periodic = sim->periodic.get();
    const int num = sim->particle_num;
    auto * kernel = sim->kernel.get();
    auto * tree = sim->tree.get();

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        std::vector<int> neighbor_list(this->m_neighbor_number * neighbor_list_size);
        
        // neighbor search
#ifdef EXHAUSTIVE_SEARCH
        int const n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, this->m_neighbor_number * neighbor_list_size, periodic, true);
#else
        int const n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, true);
#endif

        // fluid force
        const Vector<Dim> & r_i = p_i.pos;
        const Vector<Dim> & v_i = p_i.vel;
        const real gamma2_u_i = sqr(this->m_adiabatic_index - 1.0) * p_i.ene;
        const real gamma2_u_per_pres_i = gamma2_u_i / p_i.pres;
        const real m_u_inv = 1.0 / (p_i.mass * p_i.ene);
        const real h_i = p_i.sml;
        const real gradh_i = p_i.gradh;

        Vector<Dim> acc{};  // Default constructor initializes to zero
        real dene = 0.0;

        for(int n = 0; n < n_neighbor; ++n) {
            int const j = neighbor_list[n];
            auto & p_j = particles[j];
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= std::max(h_i, p_j.sml) || r == 0.0) {
                continue;
            }

            const Vector<Dim> dw_i = kernel->dw(r_ij, r, h_i);
            const Vector<Dim> dw_j = kernel->dw(r_ij, r, p_j.sml);
            const Vector<Dim> dw_ij = (dw_i + dw_j) * 0.5;
            const Vector<Dim> v_ij = v_i - p_j.vel;
            const real f_ij = 1.0 - gradh_i / (p_j.mass * p_j.ene);
            const real f_ji = 1.0 - p_j.gradh * m_u_inv;
            const real u_per_pres_j = p_j.ene / p_j.pres;

            // Use modular artificial viscosity
            algorithms::viscosity::ViscosityState<Dim> visc_state{p_i, p_j, r_ij, r};
            const real pi_ij = this->m_artificial_viscosity->compute(visc_state);
            const real dene_ac = this->m_use_ac ? this->artificial_conductivity(p_i, p_j, r_ij, dw_ij) : 0.0;

            acc -= dw_i * (p_j.mass * (gamma2_u_per_pres_i * p_j.ene * f_ij + 0.5 * pi_ij)) + dw_j * (p_j.mass * (gamma2_u_i * u_per_pres_j * f_ji + 0.5 * pi_ij));
            dene += p_j.mass * gamma2_u_per_pres_i * p_j.ene * f_ij * inner_product(v_ij, dw_i) + 0.5 * p_j.mass * pi_ij * inner_product(v_ij, dw_ij) + dene_ac;
        }

        p_i.acc = acc;
        p_i.dene = dene;
    }
}

}
}
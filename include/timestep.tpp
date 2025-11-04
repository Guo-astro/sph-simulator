#include "parameters.hpp"
#include "timestep.hpp"
#include "core/sph_particle.hpp"
#include "core/simulation.hpp"
#include "core/vector.hpp"
#include "openmp.hpp"

#include <algorithm>

namespace sph
{

template<int Dim>
void TimeStep<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    m_cfl_sound = param->cfl.sound;
    m_cfl_force = param->cfl.force;
}

template<int Dim>
void TimeStep<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    auto & particles = sim->particles;
    const int num = sim->particle_num;

    omp_real dt_min(std::numeric_limits<real>::max());
#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        const real acc_abs = abs(particles[i].acc);
        if(acc_abs > 0.0) {
            const real dt_force_i = m_cfl_force * std::sqrt(particles[i].sml / acc_abs);
            if(dt_force_i < dt_min.get()) {
                dt_min.get() = dt_force_i;
            }
        }
    }

    const real dt_sound_i = m_cfl_sound * sim->h_per_v_sig;
    
    sim->dt = std::min(dt_sound_i, dt_min.min());
}

}
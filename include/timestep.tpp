#include "parameters.hpp"
#include "timestep.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/simulation/simulation.hpp"
#include "core/utilities/vector.hpp"
#include "openmp.hpp"
#include "defines.hpp"

#include <algorithm>
#include <cmath>

namespace sph
{

template<int Dim>
void TimeStep<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    m_cfl_sound = param->get_cfl().sound;
    m_cfl_force = param->get_cfl().force;
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
        
        // Guard against division by zero and ensure acceleration is finite
        if(acc_abs > 0.0 && std::isfinite(acc_abs)) {
            const real sml = particles[i].sml;
            
            // Additional safety: check that sml is positive and finite
            if(sml > 0.0 && std::isfinite(sml)) {
                const real dt_force_i = m_cfl_force * std::sqrt(sml / acc_abs);
                
                // Ensure computed dt is finite before comparing
                if(std::isfinite(dt_force_i) && dt_force_i > 0.0 && dt_force_i < dt_min.get()) {
                    dt_min.get() = dt_force_i;
                }
            }
        }
    }

    const real dt_sound_i = m_cfl_sound * sim->h_per_v_sig;
    
    // Final safety check: ensure both dt values are finite
    real final_dt = std::numeric_limits<real>::max();
    
    if(std::isfinite(dt_sound_i) && dt_sound_i > 0.0) {
        final_dt = std::min(final_dt, dt_sound_i);
    }
    
    const real dt_force_min = dt_min.min();
    if(std::isfinite(dt_force_min) && dt_force_min > 0.0) {
        final_dt = std::min(final_dt, dt_force_min);
    }
    
    // If we still have max value, something went wrong - use a small fallback
    if(final_dt == std::numeric_limits<real>::max()) {
        WRITE_LOG << "WARNING: Unable to compute valid timestep, using fallback value";
        final_dt = 1.0e-6;
    }
    
    sim->dt = final_dt;
}

}
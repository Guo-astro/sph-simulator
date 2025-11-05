#include "defines.hpp"
#include "gravity_force.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"
#include "core/simulation.hpp"
#include "core/bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
#include "exhaustive_search.hpp"
#endif

namespace sph
{

namespace {
 // Hernquist & Katz (1989)
inline real f(const real r, const real h)
{
    const real e = h * 0.5;
    const real u = r / e;
    
    if(u < 1.0) {
        return (-0.5 * u * u * (1.0 / 3.0 - 3.0 / 20 * u * u + u * u * u / 20) + 1.4) / e;
    } else if(u < 2.0) {
        return -1.0 / (15 * r) + (-u * u * (4.0 / 3.0 - u + 0.3 * u * u - u * u * u / 30) + 1.6) / e;
    } else {
        return 1 / r;
    }
}

inline real g(const real r, const real h)
{
    const real e = h * 0.5;
    const real u = r / e;
    
    if(u < 1.0) {
        return (4.0 / 3.0 - 1.2 * u * u + 0.5 * u * u * u) / (e * e * e);
    } else if(u < 2.0) {
        return (-1.0 / 15 + 8.0 / 3 * u * u * u - 3 * u * u * u * u + 1.2 * u * u * u * u * u - u * u * u * u * u * u / 6.0) / (r * r * r);
    } else {
        return 1 / (r * r * r);
    }
}
} // anonymous namespace

template<int Dim>
void GravityForce<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    m_is_valid = param->gravity.is_valid;
    if(m_is_valid) {
        m_constant = param->gravity.constant;
    }
}

template<int Dim>
void GravityForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    if(!m_is_valid) {
        return;
    }

    auto & particles = sim->particles;
    const int num = sim->particle_num;
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
    auto * periodic = sim->periodic.get();
    
    // Use cached combined particle list (built when tree was created)
    auto & search_particles = sim->cached_search_particles;

#pragma omp parallel for
    const int search_num = sim->get_total_particle_count();
#else
    auto * tree = sim->tree.get();
#endif

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {  // Only iterate over real particles for force updates
        auto & p_i = particles[i];
        
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
        const int search_num = static_cast<int>(search_particles.size());
        real phi = 0.0;
        Vector<Dim> force(0.0);
        const Vector<Dim> & r_i = p_i.pos;

        for(int j = 0; j < search_num; ++j) {  // Search includes ghost particles
            const auto & p_j = search_particles[j];  // Access from combined list
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);
            phi -= m_constant * p_j.mass * (f(r, p_i.sml) + f(r, p_j.sml)) * 0.5;
            force -= r_ij * (m_constant * p_j.mass * (g(r, p_i.sml) + g(r, p_j.sml)) * 0.5);
        }

        p_i.acc += force;
        p_i.phi = phi;
#else
        tree->tree_force(p_i);
#endif
    }
}

}

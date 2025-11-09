#include "defines.hpp"
#include "gravity_force.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/periodic.hpp"
#include "core/simulation/simulation.hpp"
#include "core/spatial/bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
#include "exhaustive_search.hpp"
#endif

namespace sph
{

namespace {
 // Hernquist & Katz (1989)
inline real f(const real r, const real h)
{
    // Guard against division by zero
    constexpr real r_min = 1.0e-10;
    if (r < r_min) {
        return (-0.5 * (1.0 / 3.0) + 1.4) / (h * 0.5);
    }
    
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
    // Guard against division by zero
    constexpr real r_min = 1.0e-10;
    if (r < r_min) {
        return (4.0 / 3.0) / (h * h * h * 0.125);
    }
    
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
    // ✅ TYPE-SAFE: Store gravity variant directly - no binary flags
    m_gravity = param->get_gravity();
}

template<int Dim>
void GravityForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    // ✅ TYPE-SAFE: Pattern match on gravity variant - no m_is_valid check
    std::visit([&sim](auto&& g) {
        using T = std::decay_t<decltype(g)>;
        if constexpr (std::is_same_v<T, SPHParameters::NewtonianGravity>) {
            // Newtonian gravity: use Barnes-Hut tree
            auto& particles = sim->particles;
            const int num = sim->particle_num;
            
            auto* tree = sim->tree.get();
            
#pragma omp parallel for
            for(int i = 0; i < num; ++i) {
                auto& p_i = particles[i];
                tree->tree_force(p_i);
            }
        }
        // NoGravity or ModifiedGravity: do nothing (no force calculation)
    }, m_gravity);
}

}

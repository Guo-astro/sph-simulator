#include "defines.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/periodic.hpp"
#include "core/simulation/simulation.hpp"
#include "core/spatial/bhtree.hpp"
#include "core/kernels/kernel_function.hpp"
#include "gsph/g_fluid_force.hpp"
#include "algorithms/riemann/hll_solver.hpp"
#include "algorithms/limiters/van_leer_limiter.hpp"
#include "utilities/constants.hpp"

#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
#include "exhaustive_search.hpp"
#endif

namespace sph
{
namespace gsph
{

template<int Dim>
void FluidForce<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    sph::FluidForce<Dim>::initialize(param);
    this->m_is_2nd_order = param->get_gsph().is_2nd_order;
    this->m_adiabatic_index = param->get_physics().gamma;

    // Create HLL Riemann solver for interface state computation
    this->m_riemann_solver = std::make_unique<algorithms::riemann::HLLSolver>();
    
    // Create Van Leer slope limiter for MUSCL reconstruction
    this->m_slope_limiter = std::make_unique<algorithms::limiters::VanLeerLimiter>();
}

// Cha & Whitworth (2003)
template<int Dim>
void FluidForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    // Validate particle arrays in debug builds
    sim->validate_particle_arrays();
    
    auto & particles = sim->particles;
    auto * periodic = sim->periodic.get();
    const int num = sim->particle_num;
    auto * kernel = sim->kernel.get();
    auto * tree = sim->tree.get();
    const real dt = sim->dt;
    
    // Create type-safe neighbor accessor - ONLY accepts SearchParticleArray
    auto neighbor_accessor = sim->create_neighbor_accessor();

    // for MUSCL - only access gradient arrays if 2nd order is enabled
    std::vector<Vector<Dim>> *grad_d_ptr = nullptr;
    std::vector<Vector<Dim>> *grad_p_ptr = nullptr;
    Vector<Dim> * grad_v[Dim] = {nullptr};
    
    if (this->m_is_2nd_order) {
        grad_d_ptr = &sim->get_vector_array("grad_density");
        grad_p_ptr = &sim->get_vector_array("grad_pressure");
        grad_v[0] = sim->get_vector_array("grad_velocity_0").data();
        if constexpr (Dim >= 2) {
            grad_v[1] = sim->get_vector_array("grad_velocity_1").data();
        }
        if constexpr (Dim == 3) {
            grad_v[2] = sim->get_vector_array("grad_velocity_2").data();
        }
    }

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        
        // Create search config once (declarative)
        const auto search_config = NeighborSearchConfig::create(this->m_neighbor_number, /*is_ij=*/true);
        
        // Perform neighbor search using declarative API
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
        // Exhaustive search fallback for debugging
        auto & search_particles = sim->cached_search_particles;
        std::vector<int> neighbor_list(search_config.max_neighbors);
        const int search_count = static_cast<int>(search_particles.size());
        int const n_neighbor = exhaustive_search(p_i, p_i.sml, search_particles, search_count, 
                                                 neighbor_list, search_config.max_neighbors, periodic, true);
        auto result = NeighborSearchResult{
            .neighbor_indices = std::vector<int>(neighbor_list.begin(), neighbor_list.begin() + n_neighbor),
            .is_truncated = false,
            .total_candidates_found = n_neighbor
        };
#else
        // REFACTORED: Declarative neighbor search (impossible to overflow)
        auto result = tree->find_neighbors(p_i, search_config);
        
        // Diagnostic: Log if truncation occurred
        if (result.is_truncated) {
            #pragma omp critical
            {
                static bool truncation_logged = false;
                if (!truncation_logged) {
                    WRITE_LOG << "WARNING: Particle " << i 
                              << " has more neighbors than capacity ("
                              << result.total_candidates_found << " > "
                              << search_config.max_neighbors << ")";
                    truncation_logged = true;
                }
            }
        }
#endif

        // fluid force calculation
        const Vector<Dim> & r_i = p_i.pos;
        const Vector<Dim> & v_i = p_i.vel;
        const real h_i = p_i.sml;
        const real rho2_inv_i = utilities::constants::ONE / sqr(p_i.dens);

        Vector<Dim> acc{};  // Default constructor initializes to zero
        real dene = utilities::constants::ZERO;

        // TYPE-SAFE: Iterate with type-safe iterator yielding NeighborIndex
        for(auto neighbor_idx : result) {
            // TYPE-SAFE: Access neighbor through accessor - compile error if wrong array type
            const auto & p_j = neighbor_accessor.get_neighbor(neighbor_idx);
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= std::max(h_i, p_j.sml) || r == utilities::constants::ZERO) {
                continue;
            }

            const real r_inv = utilities::constants::ONE / r;
            const Vector<Dim> e_ij = r_ij * r_inv;
            const real ve_i = inner_product(v_i, e_ij);
            const real ve_j = inner_product(p_j.vel, e_ij);
            real vstar, pstar;

            // Check if neighbor is a ghost particle
            // Ghost particles don't have gradient arrays, so use 1st order for them
            const bool is_ghost = (p_j.type == static_cast<int>(ParticleType::GHOST));
            
            // CRITICAL FIX: neighbor_idx() >= num means accessing ghost particles
            // Gradient arrays (grad_d, grad_p, grad_v) are ONLY sized for [0, num)
            // Accessing j >= num will cause out-of-bounds memory access
            // Must check BOTH is_ghost flag AND j < num for safety
            const bool use_2nd_order = this->m_is_2nd_order && !is_ghost && (neighbor_idx() < num);

            if(use_2nd_order) {
                // Murante et al. (2011) - 2nd order MUSCL reconstruction
                // Access gradient arrays via pointers (only valid when 2nd order is enabled)
                auto & grad_d = *grad_d_ptr;
                auto & grad_p = *grad_p_ptr;

                real right[4], left[4];
                const real delta_i = utilities::constants::MUSCL_EXTRAPOLATION_COEFF * (utilities::constants::ONE - p_i.sound * dt * r_inv);
                const real delta_j = utilities::constants::MUSCL_EXTRAPOLATION_COEFF * (utilities::constants::ONE - p_j.sound * dt * r_inv);

                // velocity
                const real dv_ij = ve_i - ve_j;
                Vector<Dim> dv_i, dv_j;
                for(int k = 0; k < Dim; ++k) {
                    dv_i[k] = inner_product(grad_v[k][i], e_ij);
                    dv_j[k] = inner_product(grad_v[k][neighbor_idx()], e_ij);
                }
                const real dve_i = inner_product(dv_i, e_ij) * r;
                const real dve_j = inner_product(dv_j, e_ij) * r;
                right[0] = ve_i - this->m_slope_limiter->limit(dv_ij, dve_i) * delta_i;
                left[0] = ve_j + this->m_slope_limiter->limit(dv_ij, dve_j) * delta_j;

                // density
                const real dd_ij = p_i.dens - p_j.dens;
                const real dd_i = inner_product(grad_d[i], e_ij) * r;
                const real dd_j = inner_product(grad_d[neighbor_idx()], e_ij) * r;
                right[1] = p_i.dens - this->m_slope_limiter->limit(dd_ij, dd_i) * delta_i;
                left[1] = p_j.dens + this->m_slope_limiter->limit(dd_ij, dd_j) * delta_j;

                // pressure
                const real dp_ij = p_i.pres - p_j.pres;
                const real dp_i = inner_product(grad_p[i], e_ij) * r;
                const real dp_j = inner_product(grad_p[neighbor_idx()], e_ij) * r;
                right[2] = p_i.pres - this->m_slope_limiter->limit(dp_ij, dp_i) * delta_i;
                left[2] = p_j.pres + this->m_slope_limiter->limit(dp_ij, dp_j) * delta_j;

                // sound speed
                right[3] = std::sqrt(this->m_adiabatic_index * right[2] / right[1]);
                left[3] = std::sqrt(this->m_adiabatic_index * left[2] / left[1]);

                // Solve Riemann problem using modular solver
                algorithms::riemann::RiemannState left_state{left[0], left[1], left[2], left[3]};
                algorithms::riemann::RiemannState right_state{right[0], right[1], right[2], right[3]};
                auto solution = this->m_riemann_solver->solve(left_state, right_state);
                pstar = solution.pressure;
                vstar = solution.velocity;
            } else {
                const real right[4] = {
                    ve_i,
                    p_i.dens,
                    p_i.pres,
                    p_i.sound,
                };
                const real left[4] = {
                    ve_j,
                    p_j.dens,
                    p_j.pres,
                    p_j.sound,
                };

                // Solve Riemann problem using modular solver
                algorithms::riemann::RiemannState left_state{left[0], left[1], left[2], left[3]};
                algorithms::riemann::RiemannState right_state{right[0], right[1], right[2], right[3]};
                auto solution = this->m_riemann_solver->solve(left_state, right_state);
                pstar = solution.pressure;
                vstar = solution.velocity;
            }

            const Vector<Dim> dw_i = kernel->dw(r_ij, r, h_i);
            const Vector<Dim> dw_j = kernel->dw(r_ij, r, p_j.sml);
            const Vector<Dim> v_ij = e_ij * vstar;
            const real rho2_inv_j = utilities::constants::ONE / sqr(p_j.dens);
            const Vector<Dim> f = dw_i * (p_j.mass * pstar * rho2_inv_i) + dw_j * (p_j.mass * pstar * rho2_inv_j);

            acc -= f;
            dene -= inner_product(f, v_ij - v_i);
        }

        p_i.acc = acc;
        p_i.dene = dene;
    }
}

}
}
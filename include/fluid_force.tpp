#include "defines.hpp"
#include "fluid_force.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"
#include "core/simulation.hpp"
#include "core/bhtree.hpp"
#include "core/kernel_function.hpp"
#include "algorithms/viscosity/monaghan_viscosity.hpp"

#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
#include "exhaustive_search.hpp"
#endif

namespace sph
{

template<int Dim>
void FluidForce<Dim>::initialize(std::shared_ptr<SPHParameters> param)
{
    m_neighbor_number = param->physics.neighbor_number;
    m_use_ac = param->ac.is_valid;
    if(m_use_ac) {
        m_alpha_ac = param->ac.alpha;
        m_use_gravity = param->gravity.is_valid;
    }
    
    // Initialize modular artificial viscosity
    m_artificial_viscosity = std::make_unique<algorithms::viscosity::MonaghanViscosity<Dim>>(
        param->av.use_balsara_switch
    );
}

template<int Dim>
void FluidForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim)
{
    // ULTRA MINIMAL - Just a volatile write to prevent optimization
    volatile int entry_marker = 42;
    (void)entry_marker;  // Prevent unused variable warning
    
    // CRITICAL: Use stderr instead of WRITE_LOG to avoid any macro issues
    std::cerr << ">>> FluidForce::calculation ENTRY (stderr)" << std::endl;
    
    WRITE_LOG << "    FluidForce::calculation ENTRY (WRITE_LOG)";
    
    static int calc_call_count = 0;
    std::cerr << ">>> After static var decl" << std::endl;
    
    WRITE_LOG << "    After static var increment";
    ++calc_call_count;
    
    std::cerr << ">>> calc_call_count = " << calc_call_count << std::endl;
    
    WRITE_LOG << "    calc_call_count = " << calc_call_count;
    
    if (!sim) {
        WRITE_LOG << "ERROR: FluidForce::calculation called with null sim!";
        return;
    }
    
    WRITE_LOG << "    sim pointer valid, getting particles...";
    
    auto & particles = sim->particles;
    WRITE_LOG << "    Got particles reference";
    
    auto * periodic = sim->periodic.get();
    WRITE_LOG << "    Got periodic pointer";
    
    const int num = sim->particle_num;
    WRITE_LOG << "    num = " << num;
    
    auto * kernel = sim->kernel.get();
    auto * tree = sim->tree.get();

    // Use cached combined particle list (built when tree was created)
    auto & search_particles = sim->cached_search_particles;
    const int search_size = static_cast<int>(search_particles.size());
    
    WRITE_LOG << "    search_size = " << search_size;

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {  // Only iterate over real particles for force updates
        auto & p_i = particles[i];
        
        // neighbor search (searches in real + ghost particles)
#ifdef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
        std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
        const int search_num = static_cast<int>(search_particles.size());
        int const n_neighbor = exhaustive_search<Dim>(p_i, p_i.sml, search_particles, search_num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, true);
        auto result = NeighborSearchResult{neighbor_list, false, n_neighbor};
#else
        const auto search_config = NeighborSearchConfig::create(m_neighbor_number, true);
        auto result = tree->find_neighbors(p_i, search_config);
#endif

        // fluid force
        const Vector<Dim> & r_i = p_i.pos;
        const Vector<Dim> & v_i = p_i.vel;
        const real p_per_rho2_i = p_i.pres / sqr(p_i.dens);
        const real h_i = p_i.sml;
        const real gradh_i = p_i.gradh;

        Vector<Dim> acc{};  // Default constructor initializes to zero
        real dene = 0.0;

        for(int n = 0; n < static_cast<int>(result.neighbor_indices.size()); ++n) {
            int const j = result.neighbor_indices[n];
            
            // CRITICAL: Bounds check before accessing
            if (j < 0 || j >= search_size) {
#pragma omp critical
                {
                    WRITE_LOG << "ERROR in FluidForce: Particle " << i << " has neighbor index " << j 
                             << " which is out of bounds [0, " << search_size << ")"
                             << " at call #" << calc_call_count;
                }
                continue;  // Skip this neighbor
            }
            
            auto & p_j = search_particles[j];  // Access from combined list
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= std::max(h_i, p_j.sml) || r == 0.0) {
                continue;
            }

            const Vector<Dim> dw_i = kernel->dw(r_ij, r, h_i);
            const Vector<Dim> dw_j = kernel->dw(r_ij, r, p_j.sml);
            const Vector<Dim> dw_ij = (dw_i + dw_j) * 0.5;
            const Vector<Dim> v_ij = v_i - p_j.vel;

            // Use modular artificial viscosity
            algorithms::viscosity::ViscosityState<Dim> visc_state{p_i, p_j, r_ij, r};
            const real pi_ij = m_artificial_viscosity->compute(visc_state);
            const real dene_ac = m_use_ac ? artificial_conductivity(p_i, p_j, r_ij, dw_ij) : 0.0;

#if 0
            acc -= dw_ij * (p_j.mass * (p_per_rho2_i + p_j.pres / sqr(p_j.dens) + pi_ij));
            dene += p_j.mass * (p_per_rho2_i + 0.5 * pi_ij) * inner_product(v_ij, dw_ij);
#else
            acc -= dw_i * (p_j.mass * (p_per_rho2_i * gradh_i + 0.5 * pi_ij)) + dw_j * (p_j.mass * (p_j.pres / sqr(p_j.dens) * p_j.gradh + 0.5 * pi_ij));
            dene += p_j.mass * p_per_rho2_i * gradh_i * inner_product(v_ij, dw_i) + 0.5 * p_j.mass * pi_ij * inner_product(v_ij, dw_ij) + dene_ac;
#endif
        }

        p_i.acc = acc;
        p_i.dene = dene;
    }
    
    WRITE_LOG << "    FluidForce::calculation complete for call #" << calc_call_count;
}

// Artificial conductivity (Wadsley et al. 2008, Price 2008)
template<int Dim>
real FluidForce<Dim>::artificial_conductivity(const SPHParticle<Dim> & p_i, const SPHParticle<Dim> & p_j, const Vector<Dim> & r_ij, const Vector<Dim> & dw_ij)
{
    // Wadsley et al. (2008) or Price (2008)
    const real v_sig = m_use_gravity ?
        std::abs(inner_product(p_i.vel - p_j.vel, r_ij) / abs(r_ij)) :
        std::sqrt(utilities::constants::AC_PRESSURE_COEFF * std::abs(p_i.pres - p_j.pres) / (p_i.dens + p_j.dens));

    return m_alpha_ac * p_j.mass * v_sig * (p_i.ene - p_j.ene) * inner_product(dw_ij, r_ij) / abs(r_ij);
}

}
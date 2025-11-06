#pragma once

#include "module.hpp"
#include "core/utilities/vector.hpp"
#include "core/particles/sph_particle.hpp"

namespace sph
{

template<int Dim>
class PreInteraction : public Module<Dim> {
protected:
    int  m_neighbor_number;
    bool m_use_balsara_switch;
    bool m_use_time_dependent_av;
    real m_alpha_max;
    real m_alpha_min;
    real m_epsilon;
    bool m_iterative_sml;
    
    real m_adiabatic_index;  // γ (gamma) in ideal gas EOS: P = (γ-1)ρe
    bool m_iteration;
    real m_kernel_ratio;
    bool m_first;

protected:
    void initial_smoothing(std::shared_ptr<Simulation<Dim>> sim);
    
    virtual real newton_raphson(
        const SPHParticle<Dim> & p_i,
        const std::vector<SPHParticle<Dim>> & particles,
        const std::vector<int> & neighbor_list,
        const int n_neighbor,
        const Periodic<Dim> * periodic,
        const KernelFunction<Dim> * kernel
    );

public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using PreInteraction1D = PreInteraction<1>;
using PreInteraction2D = PreInteraction<2>;
using PreInteraction3D = PreInteraction<3>;

}

#include "pre_interaction.tpp"

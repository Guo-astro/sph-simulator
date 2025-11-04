#pragma once

#include "pre_interaction.hpp"

namespace sph
{
namespace disph
{

template<int Dim>
class PreInteraction : public sph::PreInteraction<Dim> {
    real newton_raphson (
        const SPHParticle<Dim> & p_i,
        const std::vector<SPHParticle<Dim>> & particles,
        const std::vector<int> & neighbor_list,
        const int n_neighbor,
        const Periodic<Dim> * periodic,
        const KernelFunction<Dim> * kernel
    ) override;

public:
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using PreInteraction1D = PreInteraction<1>;
using PreInteraction2D = PreInteraction<2>;
using PreInteraction3D = PreInteraction<3>;

}
}

#include "d_pre_interaction.tpp"

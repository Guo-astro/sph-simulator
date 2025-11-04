#pragma once

#include "pre_interaction.hpp"

namespace sph
{
namespace gsph
{

template<int Dim>
class PreInteraction : public sph::PreInteraction<Dim> {
    bool m_is_2nd_order;
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using PreInteraction1D = PreInteraction<1>;
using PreInteraction2D = PreInteraction<2>;
using PreInteraction3D = PreInteraction<3>;

}
}

#include "g_pre_interaction.tpp"

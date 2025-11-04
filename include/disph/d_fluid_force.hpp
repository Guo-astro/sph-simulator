#pragma once

#include "fluid_force.hpp"

namespace sph
{
namespace disph
{

template<int Dim>
class FluidForce : public sph::FluidForce<Dim> {
    real m_adiabatic_index;  // γ (gamma) in ideal gas EOS: P = (γ-1)ρe
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using FluidForce1D = FluidForce<1>;
using FluidForce2D = FluidForce<2>;
using FluidForce3D = FluidForce<3>;

}
}

#include "d_fluid_force.tpp"

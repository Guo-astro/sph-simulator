#pragma once

#include "fluid_force.hpp"

namespace sph
{
namespace disph
{

template<int Dim>
class FluidForce : public sph::FluidForce<Dim> {
    real m_gamma;
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

#pragma once

#include <functional>
#include "fluid_force.hpp"

namespace sph
{
namespace gsph
{

template<int Dim>
class FluidForce : public sph::FluidForce<Dim> {
    bool m_is_2nd_order;
    real m_gamma;

    // (velocity, density, pressure, sound speed)
    std::function<void(const real[], const real[], real & pstar, real & vstar)> m_solver;

    void hll_solver();
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using FluidForce1D = FluidForce<1>;
using FluidForce2D = FluidForce<2>;
using FluidForce3D = FluidForce<3>;

}
}

#include "g_fluid_force.tpp"

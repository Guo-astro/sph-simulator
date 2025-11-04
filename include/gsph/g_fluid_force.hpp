#pragma once

#include <memory>
#include "fluid_force.hpp"
#include "algorithms/riemann/riemann_solver.hpp"
#include "algorithms/limiters/slope_limiter.hpp"

namespace sph
{
namespace gsph
{

template<int Dim>
class FluidForce : public sph::FluidForce<Dim> {
    bool m_is_2nd_order;
    real m_gamma;

    // Riemann solver for interface state computation
    std::unique_ptr<algorithms::riemann::RiemannSolver> m_riemann_solver;
    
    // Slope limiter for MUSCL reconstruction
    std::unique_ptr<algorithms::limiters::SlopeLimiter> m_slope_limiter;

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

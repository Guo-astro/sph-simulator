#pragma once

#include "module.hpp"
#include "core/vector.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"
#include "algorithms/viscosity/artificial_viscosity.hpp"
#include <memory>

namespace sph
{

template<int Dim>
class FluidForce : public Module<Dim> {
protected:
    int  m_neighbor_number;
    bool m_use_ac;
    real m_alpha_ac;
    bool m_use_gravity;
    
    // Modular artificial viscosity
    std::unique_ptr<algorithms::viscosity::ArtificialViscosity<Dim>> m_artificial_viscosity;

    real artificial_conductivity(const SPHParticle<Dim> & p_i, const SPHParticle<Dim> & p_j, const Vector<Dim> & r_ij, const Vector<Dim> & dw_ij);

public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using FluidForce1D = FluidForce<1>;
using FluidForce2D = FluidForce<2>;
using FluidForce3D = FluidForce<3>;

}

#include "fluid_force.tpp"

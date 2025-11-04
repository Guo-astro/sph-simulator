#pragma once

#include "module.hpp"
#include "core/vector.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"

namespace sph
{

template<int Dim>
class GravityForce : public Module<Dim> {
protected:
    bool m_use_gravity;
    real m_gravity_constant;
    real m_gravity_theta;
    
    bool m_is_valid;
    real m_constant;

public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using GravityForce1D = GravityForce<1>;
using GravityForce2D = GravityForce<2>;
using GravityForce3D = GravityForce<3>;

}

#include "gravity_force.tpp"

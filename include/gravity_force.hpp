#pragma once

#include "module.hpp"
#include "core/utilities/vector.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/periodic.hpp"
#include "parameters.hpp"

namespace sph
{

template<int Dim>
class GravityForce : public Module<Dim> {
protected:
    // ✅ TYPE-SAFE: Store gravity variant instead of binary flags
    // No m_is_valid boolean needed - we discriminate with std::visit
    SPHParameters::GravityVariant m_gravity;

public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using GravityForce1D = GravityForce<1>;
using GravityForce2D = GravityForce<2>;
using GravityForce3D = GravityForce<3>;

}

#include "gravity_force.tpp"

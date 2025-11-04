#pragma once

#include "module.hpp"

namespace sph
{

template<int Dim>
class TimeStep : public Module<Dim> {
protected:
    real m_cfl_sound;
    real m_cfl_force;

public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
};

using TimeStep1D = TimeStep<1>;
using TimeStep2D = TimeStep<2>;
using TimeStep3D = TimeStep<3>;

}

#include "timestep.tpp"

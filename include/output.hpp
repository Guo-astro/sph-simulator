#pragma once

#include <fstream>
#include <memory>

#include "defines.hpp"
#include "core/sph_particle.hpp"
#include "core/simulation.hpp"

namespace sph
{

template<int Dim>
class Output {
    int m_count;
    std::ofstream m_out_energy;
public:
    Output(int count = 0);
    ~Output();
    void output_particle(std::shared_ptr<Simulation<Dim>> sim);
    void output_energy(std::shared_ptr<Simulation<Dim>> sim);
};

// Type aliases
using Output1D = Output<1>;
using Output2D = Output<2>;
using Output3D = Output<3>;

}

#include "output.tpp"

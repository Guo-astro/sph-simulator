#pragma once

#include <memory>

namespace sph
{
struct SPHParameters;
template<int Dim> class Simulation;

template<int Dim>
class Module {
public:
    virtual ~Module() = default;
    virtual void initialize(std::shared_ptr<SPHParameters> param) = 0;
    virtual void calculation(std::shared_ptr<Simulation<Dim>> sim) = 0;
};
}

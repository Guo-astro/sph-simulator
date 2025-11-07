#include "parameters.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include <iostream>
#include <variant>

int main() {
    auto params = sph::SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.1)
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline")
        .with_gravity(1.0, 0.5)
        .as_ssph()
        .with_artificial_viscosity(1.0)
        .build();
    
    std::cout << "Has gravity: " << params->has_gravity() << std::endl;
    
    std::visit([](auto&& g) {
        using T = std::decay_t<decltype(g)>;
        if constexpr (std::is_same_v<T, sph::SPHParameters::NewtonianGravity>) {
            std::cout << "Newtonian gravity detected!\n";
            std::cout << "  G = " << g.constant << "\n";
            std::cout << "  theta = " << g.theta << "\n";
        } else if constexpr (std::is_same_v<T, sph::SPHParameters::NoGravity>) {
            std::cout << "No gravity detected\n";
        } else {
            std::cout << "Unknown gravity type\n";
        }
    }, params->get_gravity());
    
    return 0;
}

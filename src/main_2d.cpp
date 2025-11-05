/**
 * @file main_2d.cpp
 * @brief Main entry point for 2D SPH simulations
 */

#include <iostream>
#include "solver.hpp"
#include "exception.hpp"

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    sph::exception_handler([&]() {
        sph::Solver<2> solver(argc, argv);
        solver.run();
    });
    return 0;
}

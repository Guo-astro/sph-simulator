#pragma once

#include "defines.hpp"
#include "core/sph_types.hpp"
#include <array>
#include <string>

namespace sph
{

struct SPHParameters {
    // Spatial dimension (1, 2, or 3)
    int dimension = 1;

    struct Time {
        real start;
        real end;
        real output;
        real energy;
    } time;

    SPHType type;

    struct CFL {
        real sound;
        real force;
    } cfl;

    struct ArtificialViscosity {
        real alpha;
        bool use_balsara_switch;
        bool use_time_dependent_av;
        real alpha_max;
        real alpha_min;
        real epsilon; // tau = h / (epsilon * c)
    } av;

    struct ArtificialConductivity {
        real alpha;
        bool is_valid;
    } ac;

    struct Tree {
        int max_level;
        int leaf_particle_num;
    } tree;

    struct Physics {
        int neighbor_number;
        real gamma;
    } physics;

    KernelType kernel;

    bool iterative_sml;

    // Legacy periodic boundary (maintained for backward compatibility)
    struct Periodic {
        bool is_valid;
        std::array<real, 3> range_max;  // Max size for 3D, unused elements for 1D/2D
        std::array<real, 3> range_min;
    } periodic;
    
    // New flexible boundary configuration
    struct Boundary {
        bool is_valid;
        std::array<std::string, 3> types;        // "periodic", "mirror", "none" per dimension
        std::array<bool, 3> enable_lower;         // Enable lower boundary [x, y, z]
        std::array<bool, 3> enable_upper;         // Enable upper boundary [x, y, z]
        std::array<real, 3> range_min;
        std::array<real, 3> range_max;
        std::array<std::string, 3> mirror_types;  // "no_slip", "free_slip"
    } boundary;

    struct Gravity {
        bool is_valid;
        real constant;
        real theta;
    } gravity;

    struct GSPH {
        bool is_2nd_order;
    } gsph;
};

}
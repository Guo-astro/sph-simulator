#pragma once

#include <vector>

#include "defines.hpp"
#include "core/sph_particle.hpp"
#include "core/periodic.hpp"

namespace sph
{

// Exhaustive neighbor search (O(N^2) - use for testing only)
template<int Dim>
int exhaustive_search(
    SPHParticle<Dim> & p_i,
    const real kernel_size,
    const std::vector<SPHParticle<Dim>> & particles,
    const int num,
    std::vector<int> & neighbor_list,
    const int list_size,
    Periodic<Dim> const * periodic,
    const bool is_ij);

}

#include "exhaustive_search.tpp"

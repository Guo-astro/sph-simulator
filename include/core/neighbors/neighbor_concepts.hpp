#pragma once

#include "neighbor_accessor.hpp"
#include <concepts>
#include <type_traits>

namespace sph {

// Forward declaration for particle type
template<int Dim>
class SPHParticle;

namespace concepts {

/**
 * Concept: Type must provide type-safe neighbor access.
 * 
 * Enforces that a type:
 * 1. Has get_neighbor() method taking NeighborIndex
 * 2. Returns const reference to SPHParticle<Dim>
 * 3. Has particle_count() returning size
 * 
 * Example usage:
 *   template<int Dim, NeighborProvider<Dim> Accessor>
 *   void calculate_density(const Accessor& accessor) {
 *       // Compiler guarantees accessor provides correct interface
 *   }
 * 
 * @tparam T Type to check (e.g., NeighborAccessor<Dim>)
 * @tparam Dim Spatial dimension
 */
template<typename T, int Dim>
concept NeighborProvider = requires(const T provider, NeighborIndex idx) {
    // Must provide get_neighbor taking NeighborIndex
    { provider.get_neighbor(idx) } -> std::same_as<const SPHParticle<Dim>&>;
    
    // Must provide particle count
    { provider.particle_count() } -> std::convertible_to<std::size_t>;
    
    // Must be able to check if empty
    { provider.empty() } -> std::convertible_to<bool>;
};

/**
 * Concept: Type must be a valid neighbor search result.
 * 
 * Enforces that a type:
 * 1. Is iterable (has begin() and end())
 * 2. Iterator dereference yields NeighborIndex
 * 3. Has size() method
 * 
 * This allows range-based for loops:
 *   for (auto neighbor_idx : result) {
 *       const auto& p = accessor.get_neighbor(neighbor_idx);
 *   }
 * 
 * @tparam T Type to check (e.g., NeighborSearchResult)
 */
template<typename T>
concept NeighborSearchResultType = requires(T result) {
    // Must be iterable
    { result.begin() } -> std::forward_iterator;
    { result.end() } -> std::forward_iterator;
    
    // Must have size
    { result.size() } -> std::convertible_to<std::size_t>;
    
    // Iterator must yield NeighborIndex
    requires std::convertible_to<
        decltype(*result.begin()), 
        NeighborIndex
    >;
};

/**
 * Example: Compile-time enforced neighbor processing function.
 * 
 * This function signature guarantees at compile time:
 * - Accessor must provide type-safe neighbor access
 * - Result must be a valid neighbor search result
 * - Dimension consistency between accessor and particle type
 * 
 * Attempting to call with wrong types = compile error, not runtime bug.
 */
template<int Dim, NeighborProvider<Dim> Accessor, NeighborSearchResultType Result>
void process_neighbors_example(const Accessor& accessor, const Result& neighbors) {
    // This code is guaranteed to be type-safe by the concept constraints
    for (auto neighbor_idx : neighbors) {
        const auto& particle = accessor.get_neighbor(neighbor_idx);
        // ... computation with particle
        (void)particle;  // Suppress unused warning in example
    }
}

}  // namespace concepts

}  // namespace sph

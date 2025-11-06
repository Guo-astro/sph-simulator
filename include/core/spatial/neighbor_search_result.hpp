/**
 * @file neighbor_search_result.hpp
 * @brief Immutable value object representing neighbor search results
 * 
 * Part of the declarative neighbor search refactoring.
 * Replaces mutable output parameters with value semantics.
 * 
 * Design principles:
 * - Immutable: All fields are const or returned by value
 * - Value semantics: Safe to copy, move is efficient
 * - Self-validating: is_valid() checks data integrity
 * - No magic numbers: All sizes/flags explicit
 */

#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>

/**
 * @struct NeighborSearchResult
 * @brief Contains the result of a neighbor search operation
 * 
 * This struct represents the output of searching for neighbors within a spatial tree.
 * It provides information about:
 * - Which particle indices were found as neighbors
 * - Whether the search was truncated due to capacity limits
 * - How many total candidates were encountered
 * 
 * Invariants:
 * - neighbor_indices.size() <= total_candidates_found
 * - If is_truncated is true, then neighbor_indices.size() < total_candidates_found
 * - All indices in neighbor_indices should be non-negative (validated by is_valid())
 * 
 * Example usage:
 * @code
 * auto result = tree->find_neighbors(particle, config);
 * 
 * if (result.is_truncated) {
 *     LOG_WARNING("Found more neighbors than capacity allows");
 * }
 * 
 * for (int neighbor_id : result.neighbor_indices) {
 *     // Process neighbor...
 * }
 * @endcode
 */
struct NeighborSearchResult {
    /// Indices of particles that are neighbors (within kernel radius)
    std::vector<int> neighbor_indices;
    
    /// True if we hit max_neighbors limit before exhausting all candidates
    bool is_truncated;
    
    /// Total number of candidates evaluated (including rejected or truncated)
    int total_candidates_found;
    
    /**
     * @brief Check if all neighbor indices are valid (non-negative)
     * @return true if all indices >= 0, false otherwise
     * 
     * This validates data integrity. Invalid indices (negative) indicate
     * a bug in the collection process.
     */
    [[nodiscard]] bool is_valid() const {
        return std::all_of(
            neighbor_indices.begin(),
            neighbor_indices.end(),
            [](int idx) { return idx >= 0; }
        );
    }
    
    /**
     * @brief Get the number of neighbors found
     * @return Count of neighbor indices stored
     */
    [[nodiscard]] size_t size() const {
        return neighbor_indices.size();
    }
    
    /**
     * @brief Check if no neighbors were found
     * @return true if neighbor_indices is empty
     */
    [[nodiscard]] bool empty() const {
        return neighbor_indices.empty();
    }
};

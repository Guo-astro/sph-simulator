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
#include "core/neighbors/neighbor_accessor.hpp"

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
     * Type-safe iterator that returns NeighborIndex instead of raw int.
     * 
     * This iterator wraps the underlying vector<int> iterator and converts
     * each int to a NeighborIndex on dereference.
     * 
     * Enables type-safe range-based for loops:
     *   for (auto neighbor_idx : result) {
     *       const auto& p = accessor.get_neighbor(neighbor_idx);
     *   }
     */
    class NeighborIndexIterator {
    private:
        std::vector<int>::const_iterator m_iter;
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = sph::NeighborIndex;
        using difference_type = std::ptrdiff_t;
        using pointer = const sph::NeighborIndex*;
        using reference = sph::NeighborIndex;
        
        explicit NeighborIndexIterator(std::vector<int>::const_iterator iter)
            : m_iter(iter) {}
        
        /**
         * Dereference returns NeighborIndex (by value).
         * Caller must use this with NeighborAccessor::get_neighbor().
         */
        sph::NeighborIndex operator*() const {
            return sph::NeighborIndex{*m_iter};
        }
        
        NeighborIndexIterator& operator++() {
            ++m_iter;
            return *this;
        }
        
        NeighborIndexIterator operator++(int) {
            auto tmp = *this;
            ++m_iter;
            return tmp;
        }
        
        bool operator==(const NeighborIndexIterator& other) const {
            return m_iter == other.m_iter;
        }
        
        bool operator!=(const NeighborIndexIterator& other) const {
            return m_iter != other.m_iter;
        }
    };
    
    /**
     * Range-based for loop support.
     * Returns type-safe iterator that yields NeighborIndex.
     */
    NeighborIndexIterator begin() const {
        return NeighborIndexIterator{neighbor_indices.begin()};
    }
    
    NeighborIndexIterator end() const {
        return NeighborIndexIterator{neighbor_indices.end()};
    }
    
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

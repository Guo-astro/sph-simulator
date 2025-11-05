/**
 * @file neighbor_collector.hpp
 * @brief RAII-based neighbor accumulator with automatic bounds enforcement
 * 
 * Part of the declarative neighbor search refactoring.
 * Prevents heap buffer overflows by design through capacity enforcement.
 * 
 * Design principles:
 * - RAII: Resource lifetime tied to object lifetime
 * - Bounds-safe: Impossible to overflow by design
 * - Move semantics: Efficient result extraction
 * - No manual counting: Automatic state management
 */

#pragma once

#include "neighbor_search_result.hpp"
#include <vector>
#include <cstddef>

/**
 * @class NeighborCollector
 * @brief Safely collects neighbor indices with capacity enforcement
 * 
 * This class provides a safe interface for accumulating neighbor particle indices
 * during tree traversal. It automatically enforces capacity limits and prevents
 * buffer overflows that were possible with manual index management.
 * 
 * Key safety features:
 * - Pre-allocated storage (no reallocations during collection)
 * - Automatic bounds checking on every addition
 * - Rejects invalid (negative) indices
 * - Tracks total candidates vs accepted neighbors
 * 
 * Usage pattern:
 * @code
 * NeighborCollector collector(max_neighbors);
 * 
 * // During tree traversal:
 * if (!collector.is_full()) {
 *     collector.try_add(particle_id);
 * }
 * 
 * // Extract result:
 * auto result = std::move(collector).finalize();
 * @endcode
 * 
 * Design rationale:
 * - try_add() returns bool to indicate success/failure
 * - finalize() is rvalue-qualified to enforce move semantics
 * - total_candidates_ tracks all attempts, not just successes
 */
class NeighborCollector {
public:
    /**
     * @brief Construct collector with specified capacity
     * @param max_capacity Maximum number of neighbors to store
     * 
     * Pre-allocates storage to avoid reallocation during collection.
     * Capacity cannot be changed after construction (immutable).
     */
    explicit NeighborCollector(size_t max_capacity) 
        : max_capacity_(max_capacity) {
        indices_.reserve(max_capacity);
    }
    
    /**
     * @brief Attempt to add a neighbor index
     * @param neighbor_id Particle index to add
     * @return true if added successfully, false if rejected
     * 
     * Rejection reasons:
     * - Capacity already reached (is_full() == true)
     * - Invalid index (neighbor_id < 0)
     * 
     * Note: total_candidates_ is incremented regardless of success,
     * allowing detection of truncation in the final result.
     */
    [[nodiscard]] bool try_add(int neighbor_id) {
        ++total_candidates_;
        
        // Reject if at capacity
        if (indices_.size() >= max_capacity_) {
            return false;
        }
        
        // Reject invalid indices
        if (neighbor_id < 0) {
            return false;
        }
        
        indices_.push_back(neighbor_id);
        return true;
    }
    
    /**
     * @brief Check if collector is at full capacity
     * @return true if no more neighbors can be added
     * 
     * Use this for early exit during tree traversal to avoid
     * unnecessary work once capacity is reached.
     */
    [[nodiscard]] bool is_full() const {
        return indices_.size() >= max_capacity_;
    }
    
    /**
     * @brief Finalize collection and extract result (rvalue-qualified)
     * @return NeighborSearchResult with collected indices
     * 
     * This method is rvalue-qualified (&&) to enforce move semantics.
     * After calling finalize(), the collector is in a moved-from state
     * and should not be reused.
     * 
     * The result indicates truncation if total_candidates_ > indices_.size().
     */
    [[nodiscard]] NeighborSearchResult finalize() && {
        return NeighborSearchResult{
            .neighbor_indices = std::move(indices_),
            .is_truncated = (total_candidates_ > indices_.size()),
            .total_candidates_found = static_cast<int>(total_candidates_)
        };
    }
    
private:
    std::vector<int> indices_;        ///< Collected neighbor indices
    const size_t max_capacity_;       ///< Maximum allowed neighbors (immutable)
    size_t total_candidates_ = 0;     ///< Total attempts to add (for truncation detection)
};

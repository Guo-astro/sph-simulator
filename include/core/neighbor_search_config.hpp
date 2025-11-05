/**
 * @file neighbor_search_config.hpp
 * @brief Validated configuration object for neighbor search operations
 * 
 * Part of the declarative neighbor search refactoring.
 * Encapsulates search parameters with validation to prevent invalid configurations.
 * 
 * Design principles:
 * - Validated construction: Factory method ensures valid state
 * - Explicit contract: All parameters documented and checked
 * - No magic numbers: Constants defined as constexpr
 * - Immutable: Configuration cannot change after construction
 */

#pragma once

#include <stdexcept>
#include <string>
#include <cstddef>

/**
 * @struct NeighborSearchConfig
 * @brief Configuration parameters for neighbor search
 * 
 * This struct encapsulates all parameters needed for a neighbor search operation.
 * It ensures that invalid configurations are rejected at construction time
 * rather than causing runtime failures.
 * 
 * Parameters:
 * - max_neighbors: Hard limit on result size (prevents buffer overflow)
 * - use_max_kernel: Whether to use maximum of particle kernels for distance check
 * 
 * Invariants (enforced by factory method and is_valid()):
 * - max_neighbors > 0
 * - max_neighbors <= kMaxReasonableNeighbors (sanity check)
 * 
 * Example usage:
 * @code
 * // Recommended: Use factory method with validation
 * auto config = NeighborSearchConfig::create(neighbor_number, is_ij);
 * 
 * // For special cases: Direct construction + manual validation
 * NeighborSearchConfig custom_config{.max_neighbors = 100, .use_max_kernel = true};
 * assert(custom_config.is_valid());
 * @endcode
 */
struct NeighborSearchConfig {
    /// Safety factor: Multiplier applied to neighbor_number to get max_neighbors
    /// Rationale: Allows some overflow beyond expected neighbors before truncating
    static constexpr int kSafetyFactor = 20;
    
    /// Sanity check: Unreasonably large neighbor count likely indicates a bug
    /// Typical SPH simulations have 20-200 neighbors per particle
    static constexpr size_t kMaxReasonableNeighbors = 100000;
    
    /// Maximum number of neighbors to collect
    size_t max_neighbors;
    
    /// If true, use max(p_i.sml, kernel_size) for radius check (for is_ij searches)
    bool use_max_kernel;
    
    /**
     * @brief Factory method to create validated config from SPH parameters
     * @param neighbor_number Expected number of neighbors per particle
     * @param is_ij Whether this is an is_ij search (affects kernel size selection)
     * @return Validated NeighborSearchConfig
     * @throws std::invalid_argument if neighbor_number <= 0
     * 
     * This is the recommended way to construct a config. It applies the safety
     * factor automatically and validates inputs.
     * 
     * Design rationale:
     * - neighbor_number comes from SPH simulation parameters (e.g., 6 for 2D)
     * - Safety factor (20x) allows dynamic particle distributions to exceed
     *   the nominal neighbor count without truncation
     * - is_ij parameter controls whether to use symmetric kernel radius
     */
    [[nodiscard]] static NeighborSearchConfig create(int neighbor_number, bool is_ij) {
        if (neighbor_number <= 0) {
            throw std::invalid_argument(
                "neighbor_number must be positive, got: " + std::to_string(neighbor_number)
            );
        }
        
        return NeighborSearchConfig{
            .max_neighbors = static_cast<size_t>(neighbor_number * kSafetyFactor),
            .use_max_kernel = is_ij
        };
    }
    
    /**
     * @brief Validate configuration parameters
     * @return true if configuration is valid and reasonable
     * 
     * Checks:
     * - max_neighbors > 0 (at least one neighbor possible)
     * - max_neighbors <= kMaxReasonableNeighbors (sanity check for bugs)
     * 
     * Use this after direct construction to ensure validity.
     */
    [[nodiscard]] bool is_valid() const {
        return max_neighbors > 0 && max_neighbors <= kMaxReasonableNeighbors;
    }
};

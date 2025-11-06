/**
 * @file boundary_builder.hpp
 * @brief Type-safe, declarative boundary configuration builder
 * 
 * Design Philosophy:
 * - Eliminates boolean trap: No more confusing (true, min, max, true)
 * - Declarative API: Intent is clear from method names
 * - Compile-time safety: Wrong types won't compile
 * - Fluent interface: Chainable for readability
 * - Self-documenting: Code reads like specification
 * 
 * Critical Architectural Guarantee:
 * **Ghost particles are AUTOMATICALLY enabled for periodic and mirror boundaries**
 * - No way to accidentally disable ghosts for Barnes-Hut tree
 * - Compile-time prevention of architectural bugs
 * 
 * Example Usage:
 * 
 * // Old API (ERROR-PRONE):
 * auto config = BoundaryConfigHelper<1>::from_baseline_json(
 *     true, range_min, range_max,
 *     true  // What does this mean? Easy to forget!
 * );
 * 
 * // New API (TYPE-SAFE):
 * auto config = BoundaryBuilder<1>()
 *     .with_periodic_boundaries()
 *     .in_range(range_min, range_max)
 *     .build();  // Ghosts automatically enabled!
 * 
 * @author GitHub Copilot with Serena MCP
 * @date 2025-01-06
 */

#pragma once

#include "boundary_types.hpp"
#include "vector.hpp"
#include <stdexcept>
#include <string>
#include <sstream>

namespace sph {

/**
 * @brief Type-safe builder for boundary configurations
 * 
 * Provides fluent API for creating BoundaryConfiguration objects
 * with compile-time safety and clear intent.
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class BoundaryBuilder {
private:
    BoundaryConfiguration<Dim> config_;
    bool range_set_ = false;
    
public:
    /**
     * @brief Construct new builder with default state
     * 
     * Default: No boundaries, ghosts disabled
     */
    BoundaryBuilder() {
        config_.is_valid = false;
        for (int d = 0; d < Dim; ++d) {
            config_.types[d] = BoundaryType::NONE;
            config_.enable_lower[d] = false;
            config_.enable_upper[d] = false;
            config_.mirror_types[d] = MirrorType::FREE_SLIP;
            config_.spacing_lower[d] = 0.0;
            config_.spacing_upper[d] = 0.0;
        }
    }
    
    // ============================================================
    // PRIMARY API: Boundary Type Configuration
    // ============================================================
    
    /**
     * @brief Enable periodic boundaries in ALL dimensions
     * 
     * Ghost particles are AUTOMATICALLY enabled.
     * Particles wrap around domain edges.
     * 
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_periodic_boundaries() {
        config_.is_valid = true;  // Ghosts required for Barnes-Hut!
        for (int d = 0; d < Dim; ++d) {
            config_.types[d] = BoundaryType::PERIODIC;
            config_.enable_lower[d] = true;
            config_.enable_upper[d] = true;
        }
        return *this;
    }
    
    /**
     * @brief Enable periodic boundary in specific dimension
     * 
     * Ghost particles are AUTOMATICALLY enabled.
     * 
     * @param dimension Dimension index (0=X, 1=Y, 2=Z)
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_periodic_in_dimension(int dimension) {
        validate_dimension(dimension);
        config_.is_valid = true;  // Any boundary type enables ghosts
        config_.types[dimension] = BoundaryType::PERIODIC;
        config_.enable_lower[dimension] = true;
        config_.enable_upper[dimension] = true;
        return *this;
    }
    
    /**
     * @brief Enable mirror (reflective) boundaries in ALL dimensions
     * 
     * Ghost particles are AUTOMATICALLY enabled.
     * Creates reflected ghost particles at walls.
     * 
     * @param mirror_type Wall type (FREE_SLIP or NO_SLIP)
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_mirror_boundaries(MirrorType mirror_type = MirrorType::FREE_SLIP) {
        config_.is_valid = true;  // Ghosts required for mirrors!
        for (int d = 0; d < Dim; ++d) {
            config_.types[d] = BoundaryType::MIRROR;
            config_.mirror_types[d] = mirror_type;
            config_.enable_lower[d] = true;
            config_.enable_upper[d] = true;
        }
        return *this;
    }
    
    /**
     * @brief Enable mirror boundary in specific dimension
     * 
     * Ghost particles are AUTOMATICALLY enabled.
     * 
     * @param dimension Dimension index (0=X, 1=Y, 2=Z)
     * @param mirror_type Wall type (FREE_SLIP or NO_SLIP)
     * @param spacing_lower Particle spacing for lower wall ghosts
     * @param spacing_upper Particle spacing for upper wall ghosts
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_mirror_in_dimension(
        int dimension,
        MirrorType mirror_type = MirrorType::FREE_SLIP,
        double spacing_lower = 0.0,
        double spacing_upper = 0.0
    ) {
        validate_dimension(dimension);
        config_.is_valid = true;
        config_.types[dimension] = BoundaryType::MIRROR;
        config_.mirror_types[dimension] = mirror_type;
        config_.enable_lower[dimension] = true;
        config_.enable_upper[dimension] = true;
        config_.spacing_lower[dimension] = spacing_lower;
        config_.spacing_upper[dimension] = spacing_upper;
        return *this;
    }
    
    /**
     * @brief Disable boundaries in specific dimension (open boundary)
     * 
     * @param dimension Dimension index (0=X, 1=Y, 2=Z)
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_no_boundary_in_dimension(int dimension) {
        validate_dimension(dimension);
        config_.types[dimension] = BoundaryType::NONE;
        config_.enable_lower[dimension] = false;
        config_.enable_upper[dimension] = false;
        return *this;
    }
    
    /**
     * @brief Disable ALL boundaries (open boundaries)
     * 
     * Ghost particles are disabled.
     * Use for large domains where particles never reach edges.
     * 
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_no_boundaries() {
        config_.is_valid = false;
        for (int d = 0; d < Dim; ++d) {
            config_.types[d] = BoundaryType::NONE;
            config_.enable_lower[d] = false;
            config_.enable_upper[d] = false;
        }
        return *this;
    }
    
    // ============================================================
    // SPACING CONFIGURATION
    // ============================================================
    
    /**
     * @brief Set uniform particle spacing for ALL mirror boundaries
     * 
     * @param spacing Uniform spacing (dx = dy = dz)
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_uniform_spacing(double spacing) {
        for (int d = 0; d < Dim; ++d) {
            config_.spacing_lower[d] = spacing;
            config_.spacing_upper[d] = spacing;
        }
        return *this;
    }
    
    /**
     * @brief Set spacing for specific dimension
     * 
     * @param dimension Dimension index
     * @param spacing_lower Lower wall spacing
     * @param spacing_upper Upper wall spacing
     * @return Builder reference for chaining
     */
    BoundaryBuilder& with_spacing_in_dimension(
        int dimension,
        double spacing_lower,
        double spacing_upper
    ) {
        validate_dimension(dimension);
        config_.spacing_lower[dimension] = spacing_lower;
        config_.spacing_upper[dimension] = spacing_upper;
        return *this;
    }
    
    // ============================================================
    // RANGE CONFIGURATION
    // ============================================================
    
    /**
     * @brief Set domain range for boundaries
     * 
     * REQUIRED for periodic and mirror boundaries.
     * 
     * @param min Minimum bounds [x_min, y_min, z_min]
     * @param max Maximum bounds [x_max, y_max, z_max]
     * @return Builder reference for chaining
     */
    BoundaryBuilder& in_range(const Vector<Dim>& min, const Vector<Dim>& max) {
        // Validate range
        for (int d = 0; d < Dim; ++d) {
            if (min[d] >= max[d]) {
                throw std::invalid_argument(
                    "BoundaryBuilder: range_min must be less than range_max in all dimensions"
                );
            }
        }
        
        config_.range_min = min;
        config_.range_max = max;
        range_set_ = true;
        return *this;
    }
    
    // ============================================================
    // SELECTIVE BOUNDARY ENABLING
    // ============================================================
    
    /**
     * @brief Disable lower boundary in specific dimension
     * 
     * Useful for: floor-only setups, one-sided walls
     * 
     * @param dimension Dimension index
     * @return Builder reference for chaining
     */
    BoundaryBuilder& disable_lower_boundary_in_dimension(int dimension) {
        validate_dimension(dimension);
        config_.enable_lower[dimension] = false;
        return *this;
    }
    
    /**
     * @brief Disable upper boundary in specific dimension
     * 
     * Useful for: ceiling-less setups, one-sided walls
     * 
     * @param dimension Dimension index
     * @return Builder reference for chaining
     */
    BoundaryBuilder& disable_upper_boundary_in_dimension(int dimension) {
        validate_dimension(dimension);
        config_.enable_upper[dimension] = false;
        return *this;
    }
    
    // ============================================================
    // BUILD AND VALIDATE
    // ============================================================
    
    /**
     * @brief Build final configuration
     * 
     * Validates configuration and returns immutable result.
     * 
     * @return Validated BoundaryConfiguration
     * @throws std::invalid_argument If configuration is invalid
     */
    BoundaryConfiguration<Dim> build() const {
        // Validate: If any boundary is active, range must be set
        bool has_active_boundary = false;
        for (int d = 0; d < Dim; ++d) {
            if (config_.types[d] != BoundaryType::NONE) {
                has_active_boundary = true;
                break;
            }
        }
        
        if (has_active_boundary && !range_set_) {
            throw std::invalid_argument(
                "BoundaryBuilder: range must be set before building"
            );
        }
        
        return config_;
    }
    
    // ============================================================
    // STATIC FACTORY METHODS (Backwards Compatibility)
    // ============================================================
    
    /**
     * @brief Create periodic configuration (convenience method)
     * 
     * Equivalent to:
     *   BoundaryBuilder<Dim>()
     *     .with_periodic_boundaries()
     *     .in_range(min, max)
     *     .build()
     * 
     * @param min Domain minimum
     * @param max Domain maximum
     * @return Configured BoundaryConfiguration
     */
    static BoundaryConfiguration<Dim> create_periodic(
        const Vector<Dim>& min,
        const Vector<Dim>& max
    ) {
        return BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(min, max)
            .build();
    }
    
    /**
     * @brief Create mirror configuration (convenience method)
     * 
     * @param min Domain minimum
     * @param max Domain maximum
     * @param mirror_type Wall type
     * @param spacing Uniform spacing
     * @return Configured BoundaryConfiguration
     */
    static BoundaryConfiguration<Dim> create_mirror(
        const Vector<Dim>& min,
        const Vector<Dim>& max,
        MirrorType mirror_type = MirrorType::FREE_SLIP,
        double spacing = 0.0
    ) {
        return BoundaryBuilder<Dim>()
            .with_mirror_boundaries(mirror_type)
            .with_uniform_spacing(spacing)
            .in_range(min, max)
            .build();
    }
    
    /**
     * @brief Create no-boundary configuration (convenience method)
     * 
     * @return Configured BoundaryConfiguration
     */
    static BoundaryConfiguration<Dim> create_none() {
        return BoundaryBuilder<Dim>()
            .with_no_boundaries()
            .build();
    }
    
    // ============================================================
    // DESCRIPTION AND DEBUGGING
    // ============================================================
    
    /**
     * @brief Get human-readable description of configuration
     * 
     * @param config Configuration to describe
     * @return Descriptive string
     */
    static std::string describe(const BoundaryConfiguration<Dim>& config) {
        if (!config.is_valid) {
            return "Open boundaries (no ghosts)";
        }
        
        std::ostringstream oss;
        oss << "Ghost particles enabled:\n";
        
        for (int d = 0; d < Dim; ++d) {
            oss << "  Dimension " << d << ": ";
            
            switch (config.types[d]) {
                case BoundaryType::PERIODIC:
                    oss << "Periodic";
                    break;
                case BoundaryType::MIRROR:
                    oss << "Mirror (";
                    oss << (config.mirror_types[d] == MirrorType::FREE_SLIP 
                            ? "FREE_SLIP" : "NO_SLIP");
                    oss << ")";
                    break;
                case BoundaryType::NONE:
                    oss << "None";
                    break;
            }
            
            oss << " [" << config.range_min[d] << ", " << config.range_max[d] << "]";
            
            if (config.types[d] == BoundaryType::MIRROR) {
                oss << " spacing=[" << config.spacing_lower[d] 
                    << ", " << config.spacing_upper[d] << "]";
            }
            
            if (!config.enable_lower[d] && !config.enable_upper[d]) {
                oss << " (disabled)";
            } else if (!config.enable_lower[d]) {
                oss << " (upper only)";
            } else if (!config.enable_upper[d]) {
                oss << " (lower only)";
            }
            
            oss << "\n";
        }
        
        return oss.str();
    }
    
private:
    /**
     * @brief Validate dimension index
     * 
     * @param dimension Dimension to validate
     * @throws std::out_of_range If dimension invalid
     */
    void validate_dimension(int dimension) const {
        if (dimension < 0 || dimension >= Dim) {
            throw std::out_of_range(
                "BoundaryBuilder: dimension must be in range [0, " 
                + std::to_string(Dim) + ")"
            );
        }
    }
};

} // namespace sph

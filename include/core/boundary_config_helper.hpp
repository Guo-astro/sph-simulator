#pragma once

#include "core/boundary_types.hpp"
#include <string>

namespace sph {

/**
 * @brief Helper class for creating boundary configurations
 * 
 * Provides easy switching between:
 * - Baseline mode: Disables ghost particles, uses legacy periodic
 * - Modern mode: Enables ghost particles with proper filtering
 * 
 * This allows exact parameter replication of baseline commit abd7353
 * while maintaining compatibility with modern ghost particle system.
 */
template<int Dim>
class BoundaryConfigHelper {
public:
    /**
     * @brief Create baseline-compatible configuration (NO ghosts)
     * 
     * Reproduces exact behavior of baseline commit abd7353:
     * - Ghost particles disabled (is_valid = false)
     * - Legacy periodic boundary via params->periodic
     * - No ghost filtering needed in Newton-Raphson
     * 
     * Use this mode to verify that current code produces same results
     * as baseline when ghosts are disabled.
     * 
     * @param range_min Domain minimum bounds
     * @param range_max Domain maximum bounds
     * @return BoundaryConfiguration with ghosts disabled
     */
    static BoundaryConfiguration<Dim> create_baseline_mode(
        const Vector<Dim>& range_min,
        const Vector<Dim>& range_max
    ) {
        BoundaryConfiguration<Dim> config;
        
        // CRITICAL: Disable ghost particle system
        // This makes current code behave exactly like baseline
        config.is_valid = false;
        
        // Store range for legacy periodic boundary
        config.range_min = range_min;
        config.range_max = range_max;
        
        // All other fields unused when is_valid=false
        for (int d = 0; d < Dim; ++d) {
            config.types[d] = BoundaryType::NONE;
            config.enable_lower[d] = false;
            config.enable_upper[d] = false;
        }
        
        return config;
    }
    
    /**
     * @brief Create modern periodic configuration WITH ghosts
     * 
     * Uses modern ghost particle system:
     * - Ghost particles enabled (is_valid = true)
     * - PERIODIC boundary type generates wrapping ghosts
     * - Ghost filtering applied in Newton-Raphson (current fix)
     * 
     * @param range_min Domain minimum bounds
     * @param range_max Domain maximum bounds
     * @return BoundaryConfiguration with periodic ghosts
     */
    static BoundaryConfiguration<Dim> create_periodic_with_ghosts(
        const Vector<Dim>& range_min,
        const Vector<Dim>& range_max
    ) {
        BoundaryConfiguration<Dim> config;
        
        // Enable ghost particle system
        config.is_valid = true;
        
        // Set range
        config.range_min = range_min;
        config.range_max = range_max;
        
        // Configure periodic boundaries in all dimensions
        for (int d = 0; d < Dim; ++d) {
            config.types[d] = BoundaryType::PERIODIC;
            config.enable_lower[d] = true;
            config.enable_upper[d] = true;
        }
        
        return config;
    }
    
    /**
     * @brief Create modern mirror configuration WITH ghosts
     * 
     * Uses modern ghost particle system with reflective walls:
     * - Ghost particles enabled (is_valid = true)
     * - MIRROR boundary type generates reflected ghosts
     * - Ghost filtering applied in Newton-Raphson (current fix)
     * 
     * @param range_min Domain minimum bounds
     * @param range_max Domain maximum bounds
     * @param mirror_type Type of mirror (FREE_SLIP or NO_SLIP)
     * @param spacing Particle spacing for Morris 1997 wall offset
     * @return BoundaryConfiguration with mirror ghosts
     */
    static BoundaryConfiguration<Dim> create_mirror_with_ghosts(
        const Vector<Dim>& range_min,
        const Vector<Dim>& range_max,
        MirrorType mirror_type = MirrorType::FREE_SLIP,
        const Vector<Dim>& spacing = Vector<Dim>()
    ) {
        BoundaryConfiguration<Dim> config;
        
        // Enable ghost particle system
        config.is_valid = true;
        
        // Set range
        config.range_min = range_min;
        config.range_max = range_max;
        
        // Configure mirror boundaries in all dimensions
        for (int d = 0; d < Dim; ++d) {
            config.types[d] = BoundaryType::MIRROR;
            config.enable_lower[d] = true;
            config.enable_upper[d] = true;
            config.mirror_types[d] = mirror_type;
            
            // Set spacing for Morris 1997 wall offset calculation
            // If spacing not provided, will be set later from actual particles
            if (spacing[d] > 0.0) {
                config.spacing_lower[d] = spacing[d];
                config.spacing_upper[d] = spacing[d];
            }
        }
        
        return config;
    }
    
    /**
     * @brief Create no-boundary configuration (open boundaries)
     * 
     * No boundary treatment:
     * - Ghost particles disabled (is_valid = false)
     * - No periodic wrapping
     * - No reflective walls
     * 
     * Use for simulations with large domains where particles
     * never reach boundaries.
     * 
     * @return BoundaryConfiguration with no boundaries
     */
    static BoundaryConfiguration<Dim> create_no_boundary() {
        BoundaryConfiguration<Dim> config;
        
        // Disable ghost particle system
        config.is_valid = false;
        
        // All boundaries set to NONE
        for (int d = 0; d < Dim; ++d) {
            config.types[d] = BoundaryType::NONE;
            config.enable_lower[d] = false;
            config.enable_upper[d] = false;
        }
        
        return config;
    }
    
    /**
     * @brief Parse baseline JSON config to modern BoundaryConfiguration
     * 
     * Converts legacy JSON format from baseline abd7353:
     *   "periodic": true,
     *   "rangeMin": [-0.5],
     *   "rangeMax": [1.5]
     * 
     * To modern BoundaryConfiguration format:
     *   is_valid = false (baseline mode, no ghosts)
     *   range_min = [-0.5]
     *   range_max = [1.5]
     * 
     * @param periodic Legacy periodic flag
     * @param range_min Domain minimum bounds
     * @param range_max Domain maximum bounds
     * @param enable_ghosts If true, uses modern mode; if false, baseline mode
     * @return BoundaryConfiguration matching baseline behavior
     */
    static BoundaryConfiguration<Dim> from_baseline_json(
        bool periodic,
        const Vector<Dim>& range_min,
        const Vector<Dim>& range_max,
        bool enable_ghosts = false
    ) {
        if (enable_ghosts) {
            // Modern mode: Enable ghosts with proper filtering
            if (periodic) {
                return create_periodic_with_ghosts(range_min, range_max);
            } else {
                return create_no_boundary();
            }
        } else {
            // Baseline mode: Disable ghosts, use legacy system
            return create_baseline_mode(range_min, range_max);
        }
    }
    
    /**
     * @brief Get human-readable description of boundary configuration
     * 
     * @param config Boundary configuration
     * @return String describing the configuration
     */
    static std::string describe(const BoundaryConfiguration<Dim>& config) {
        if (!config.is_valid) {
            return "No ghosts (baseline mode or open boundaries)";
        }
        
        std::string desc = "Ghost particles enabled:\n";
        for (int d = 0; d < Dim; ++d) {
            desc += "  Dimension " + std::to_string(d) + ": ";
            
            switch (config.types[d]) {
                case BoundaryType::PERIODIC:
                    desc += "PERIODIC";
                    break;
                case BoundaryType::MIRROR:
                    desc += "MIRROR (";
                    desc += (config.mirror_types[d] == MirrorType::FREE_SLIP) 
                            ? "FREE_SLIP)" : "NO_SLIP)";
                    break;
                case BoundaryType::NONE:
                    desc += "NONE";
                    break;
            }
            
            desc += " [" + std::to_string(config.range_min[d]) + ", " 
                         + std::to_string(config.range_max[d]) + "]";
            
            if (config.types[d] == BoundaryType::MIRROR) {
                desc += " spacing=[" + std::to_string(config.spacing_lower[d]) 
                     + ", " + std::to_string(config.spacing_upper[d]) + "]";
            }
            
            desc += "\n";
        }
        
        return desc;
    }
};

} // namespace sph

#ifndef SPH_BOUNDARY_TYPES_HPP
#define SPH_BOUNDARY_TYPES_HPP

#include <array>
#include <string>
#include "../defines.hpp"
#include "vector.hpp"

namespace sph {

/**
 * @brief Types of boundary conditions supported in SPH simulations
 * 
 * Following the ghost particle method described in:
 * Lajoie & Sills (2010) - Mass Transfer in Binary Stars using SPH
 */
enum class BoundaryType {
    NONE,           ///< Open/free boundary (no ghost particles)
    PERIODIC,       ///< Periodic wrapping with ghost particles
    MIRROR,         ///< Wall boundary with mirror ghost particles
    FREE_SURFACE    ///< Free surface boundary (future extension)
};

/**
 * @brief Mirror boundary velocity treatment types
 */
enum class MirrorType {
    NO_SLIP,        ///< Reflect all velocity components (v → -v normal, u → -u tangential)
    FREE_SLIP       ///< Reflect only normal velocity (v → -v normal, u → u tangential)
};

/**
 * @brief Boundary configuration for multi-dimensional simulations
 * 
 * Supports flexible per-dimension boundary conditions for 1D, 2D, and 3D.
 * Each dimension can independently have different boundary types.
 * 
 * @tparam Dim Spatial dimensionality (1, 2, or 3)
 * 
 * @example 2D simulation with periodic x and mirror y boundaries:
 * @code
 * BoundaryConfiguration<2> config;
 * config.types[0] = BoundaryType::PERIODIC;
 * config.types[1] = BoundaryType::MIRROR;
 * config.enable_lower[1] = true;
 * config.enable_upper[1] = true;
 * config.mirror_types[1] = MirrorType::NO_SLIP;
 * config.range_min = {-0.5, 0.0};
 * config.range_max = {1.5, 1.0};
 * @endcode
 */
template<int Dim>
struct BoundaryConfiguration {
    bool is_valid;                                  ///< Whether boundary conditions are enabled
    std::array<BoundaryType, Dim> types;            ///< Boundary type for each dimension
    std::array<bool, Dim> enable_lower;             ///< Enable lower boundary per dimension
    std::array<bool, Dim> enable_upper;             ///< Enable upper boundary per dimension
    Vector<Dim> range_min;                          ///< Minimum coordinates of the particle domain
    Vector<Dim> range_max;                          ///< Maximum coordinates of the particle domain
    
    // Mirror boundary specific settings
    std::array<MirrorType, Dim> mirror_types;       ///< Mirror type per dimension (when type is MIRROR)
    Vector<Dim> particle_spacing;                   ///< Particle spacing per dimension (dx, dy, dz) for wall offset calculation
    
    /**
     * @brief Default constructor - initializes with no boundaries
     */
    BoundaryConfiguration() : is_valid(false) {
        for (int i = 0; i < Dim; ++i) {
            types[i] = BoundaryType::NONE;
            enable_lower[i] = false;
            enable_upper[i] = false;
            mirror_types[i] = MirrorType::NO_SLIP;
        }
    }
    
    /**
     * @brief Check if any dimension has periodic boundaries
     */
    bool has_periodic() const {
        for (int i = 0; i < Dim; ++i) {
            if (types[i] == BoundaryType::PERIODIC) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if any dimension has mirror boundaries
     */
    bool has_mirror() const {
        for (int i = 0; i < Dim; ++i) {
            if (types[i] == BoundaryType::MIRROR) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Get the range size for a given dimension
     */
    real get_range(int dim) const {
        return range_max[dim] - range_min[dim];
    }
    
    /**
     * @brief Get wall position for mirror boundaries (Morris 1997 formula)
     * 
     * Wall position is offset by ±0.5*dx from the particle domain boundary:
     * - Lower wall: x_wall = range_min - 0.5*dx
     * - Upper wall: x_wall = range_max + 0.5*dx
     * 
     * This ensures ghost particles maintain correct spacing from real particles.
     * 
     * @param dim Dimension index
     * @param is_upper True for upper boundary, false for lower
     * @return Wall position coordinate
     */
    real get_wall_position(int dim, bool is_upper) const {
        if (is_upper) {
            return range_max[dim] + 0.5 * particle_spacing[dim];
        } else {
            return range_min[dim] - 0.5 * particle_spacing[dim];
        }
    }
};

/**
 * @brief Convert string to BoundaryType enum
 */
inline BoundaryType string_to_boundary_type(const std::string& str) {
    if (str == "none") return BoundaryType::NONE;
    if (str == "periodic") return BoundaryType::PERIODIC;
    if (str == "mirror") return BoundaryType::MIRROR;
    if (str == "free_surface") return BoundaryType::FREE_SURFACE;
    return BoundaryType::NONE;
}

/**
 * @brief Convert BoundaryType enum to string
 */
inline std::string boundary_type_to_string(BoundaryType type) {
    switch (type) {
        case BoundaryType::NONE: return "none";
        case BoundaryType::PERIODIC: return "periodic";
        case BoundaryType::MIRROR: return "mirror";
        case BoundaryType::FREE_SURFACE: return "free_surface";
        default: return "none";
    }
}

/**
 * @brief Convert string to MirrorType enum
 */
inline MirrorType string_to_mirror_type(const std::string& str) {
    if (str == "no_slip") return MirrorType::NO_SLIP;
    if (str == "free_slip") return MirrorType::FREE_SLIP;
    return MirrorType::NO_SLIP;
}

/**
 * @brief Convert MirrorType enum to string
 */
inline std::string mirror_type_to_string(MirrorType type) {
    switch (type) {
        case MirrorType::NO_SLIP: return "no_slip";
        case MirrorType::FREE_SLIP: return "free_slip";
        default: return "no_slip";
    }
}

} // namespace sph

#endif // SPH_BOUNDARY_TYPES_HPP

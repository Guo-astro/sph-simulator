#pragma once

/**
 * @file sph_types.hpp
 * @brief Common type definitions for SPH simulation
 * 
 * This file contains enumerations and basic types used across
 * the SPH parameter system.
 */

namespace sph
{

/**
 * @brief SPH algorithm types
 */
enum struct SPHType {
    SSPH,    ///< Standard SPH
    DISPH,   ///< Density Independent SPH
    GSPH,    ///< Godunov SPH
};

/**
 * @brief Kernel function types
 */
enum struct KernelType {
    CUBIC_SPLINE,   ///< Cubic spline kernel (M4)
    WENDLAND,       ///< Wendland C2 kernel
    UNKNOWN,        ///< Unknown/uninitialized kernel
};

}

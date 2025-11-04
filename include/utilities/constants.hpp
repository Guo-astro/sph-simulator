#ifndef SPH_UTILITIES_CONSTANTS_HPP
#define SPH_UTILITIES_CONSTANTS_HPP

#include "defines.hpp"

namespace sph {
namespace utilities {
namespace constants {

// ============================================================================
// Mathematical Constants
// ============================================================================

constexpr real ZERO = 0.0;
constexpr real ONE = 1.0;
constexpr real TWO = 2.0;
constexpr real THREE = 3.0;
constexpr real FOUR = 4.0;
constexpr real FIVE = 5.0;
constexpr real HALF = 0.5;
constexpr real QUARTER = 0.25;
constexpr real ONE_THIRD = 1.0 / 3.0;
constexpr real TWO_THIRDS = 2.0 / 3.0;

// ============================================================================
// Physical Constants (Ideal Gas)
// ============================================================================

/** Adiabatic index for monoatomic ideal gas (argon, helium) */
constexpr real GAMMA_MONOATOMIC = 5.0 / 3.0;

/** Adiabatic index for diatomic ideal gas (air, nitrogen, oxygen) */
constexpr real GAMMA_DIATOMIC = 7.0 / 5.0;

/** Commonly used: γ - 1 for ideal gas EOS: P = (γ-1)ρe */
constexpr real GAMMA_MINUS_ONE_MONOATOMIC = GAMMA_MONOATOMIC - ONE;
constexpr real GAMMA_MINUS_ONE_DIATOMIC = GAMMA_DIATOMIC - ONE;

// ============================================================================
// Geometric Constants
// ============================================================================

/** Volume coefficient for unit sphere in 1D (length of interval [-0.5, 0.5]) */
constexpr real UNIT_SPHERE_VOLUME_1D = TWO;

/** Volume coefficient for unit sphere in 2D (area of unit circle: π) */
constexpr real UNIT_SPHERE_VOLUME_2D = M_PI;

/** Volume coefficient for unit sphere in 3D (volume: 4π/3) */
constexpr real UNIT_SPHERE_VOLUME_3D = FOUR * M_PI / THREE;

// ============================================================================
// SPH Algorithm Constants
// ============================================================================

/** MUSCL reconstruction: extrapolation distance coefficient (typically 0.5) */
constexpr real MUSCL_EXTRAPOLATION_COEFF = HALF;

/** Signal velocity estimation: coefficient for relative velocity term (typically 3.0) 
 *  Used in: v_sig = c_i + c_j - SIGNAL_VELOCITY_COEFF * (v_i - v_j)·r_ij/r
 */
constexpr real SIGNAL_VELOCITY_COEFF = THREE;

/** Default BH-tree size multiplier for memory allocation */
constexpr int BHTREE_SIZE_MULTIPLIER = 5;

/** Neighbor list size for fixed-size arrays */
constexpr int NEIGHBOR_LIST_SIZE = 20;

// ============================================================================
// Numerical Tolerances
// ============================================================================

/** Machine epsilon for double precision */
constexpr real EPSILON = 1e-15;

/** Small number for avoiding division by zero */
constexpr real TINY = 1e-10;

/** Very large number for comparisons */
constexpr real VERY_LARGE = 1e10;

/** Tolerance for floating-point comparisons */
constexpr real FLOAT_TOLERANCE = 1e-12;

} // namespace constants
} // namespace utilities
} // namespace sph

#endif // SPH_UTILITIES_CONSTANTS_HPP

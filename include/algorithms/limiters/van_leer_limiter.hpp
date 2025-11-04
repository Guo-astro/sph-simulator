/**
 * @file van_leer_limiter.hpp
 * @brief Van Leer slope limiter implementation
 * 
 * The Van Leer limiter (van Leer, 1979) is one of the most widely used slope limiters
 * for MUSCL reconstruction. It provides a good balance between accuracy and stability.
 * 
 * Formula: φ(r) = (r + |r|) / (1 + |r|) = 2r / (1 + r) for r > 0
 *         φ(r) = 0 for r ≤ 0
 * 
 * Equivalently: φ = 2*dq1*dq2 / (dq1 + dq2) if dq1*dq2 > 0, else 0
 * 
 * Properties:
 * - TVD (Total Variation Diminishing): Preserves monotonicity
 * - Symmetric: φ(r) = φ(1/r) / r
 * - Second-order accurate in smooth regions (r ≈ 1)
 * - Smoothly varying, no sharp transitions
 * - Less compressive than MinMod, less aggressive than Superbee
 * 
 * Reference:
 * van Leer, B. (1979). "Towards the ultimate conservative difference scheme. V.
 * A second-order sequel to Godunov's method." Journal of Computational Physics,
 * 32(1), 101-136.
 */

#ifndef SPH_ALGORITHMS_LIMITERS_VAN_LEER_LIMITER_HPP
#define SPH_ALGORITHMS_LIMITERS_VAN_LEER_LIMITER_HPP

#include "algorithms/limiters/slope_limiter.hpp"
#include "utilities/constants.hpp"

namespace sph {
namespace algorithms {
namespace limiters {

/**
 * @class VanLeerLimiter
 * @brief Van Leer (1979) slope limiter
 * 
 * Implements the Van Leer flux limiter for MUSCL reconstruction:
 * 
 * φ = 2 * dq1 * dq2 / (dq1 + dq2)  if dq1 * dq2 > 0
 * φ = 0                             if dq1 * dq2 ≤ 0
 * 
 * where dq1 is the upstream gradient and dq2 is the local gradient.
 * 
 * The limiter returns:
 * - Zero at local extrema (gradients with opposite signs)
 * - Harmonic mean of gradients when both have same sign
 * - Approaches the gradient value in smooth regions (second-order accuracy)
 */
class VanLeerLimiter : public SlopeLimiter {
public:
    /**
     * @brief Apply Van Leer limiting to gradients
     * 
     * @param upstream_gradient Gradient from upstream neighbor (dq1)
     * @param local_gradient Gradient from local calculation (dq2)
     * @return Limited slope value
     * 
     * Returns 2*dq1*dq2/(dq1+dq2) if same sign, 0 if opposite signs
     */
    real limit(real upstream_gradient, real local_gradient) const override
    {
        using namespace utilities::constants;
        
        // Check if gradients have same sign (product > 0)
        const real product = upstream_gradient * local_gradient;
        
        if (product <= ZERO) {
            // Extremum detected: different signs or one is zero
            return ZERO;
        }
        
        // Van Leer limiter: harmonic mean scaled by 2
        // φ = 2*dq1*dq2 / (dq1 + dq2)
        return TWO * product / (upstream_gradient + local_gradient);
    }
    
    /**
     * @brief Get the limiter name
     * @return "VanLeer"
     */
    std::string name() const override
    {
        return "VanLeer";
    }
};

} // namespace limiters
} // namespace algorithms
} // namespace sph

#endif // SPH_ALGORITHMS_LIMITERS_VAN_LEER_LIMITER_HPP

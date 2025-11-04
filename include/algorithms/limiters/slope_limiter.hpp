/**
 * @file slope_limiter.hpp
 * @brief Abstract interface for slope limiters used in MUSCL reconstruction
 * 
 * Slope limiters are essential for achieving high-order accuracy in finite volume
 * methods while maintaining stability near discontinuities. They prevent spurious
 * oscillations (Gibbs phenomenon) by limiting the reconstructed slopes.
 * 
 * A slope limiter typically:
 * - Returns 0 at local extrema (different-sign gradients) to prevent new extrema
 * - Returns the gradient value in smooth regions for second-order accuracy
 * - Provides intermediate limiting in transition regions
 * 
 * Common limiters include:
 * - Van Leer (1979): φ = 2*r/(1+r) where r = dq_upstream/dq_local
 * - MinMod: φ = min(dq1, dq2) if same sign, else 0
 * - Superbee: More aggressive, allows steeper gradients
 * - MC (Monotonized Central): Balance between MinMod and Superbee
 * 
 * All limiters should satisfy the TVD (Total Variation Diminishing) property
 * to ensure stability and monotonicity preservation.
 */

#ifndef SPH_ALGORITHMS_LIMITERS_SLOPE_LIMITER_HPP
#define SPH_ALGORITHMS_LIMITERS_SLOPE_LIMITER_HPP

#include <string>
#include "defines.hpp"

namespace sph {
namespace algorithms {
namespace limiters {

/**
 * @class SlopeLimiter
 * @brief Abstract base class for slope limiters
 * 
 * Provides a common interface for different slope limiting schemes used
 * in MUSCL (Monotonic Upstream-centered Scheme for Conservation Laws)
 * reconstruction for SPH and finite volume methods.
 */
class SlopeLimiter {
public:
    virtual ~SlopeLimiter() = default;
    
    /**
     * @brief Compute the limited slope value
     * 
     * @param upstream_gradient Gradient from upstream neighbor (dq1)
     * @param local_gradient Gradient from local calculation (dq2)
     * @return Limited slope value that preserves TVD property
     * 
     * The limiter ensures:
     * - If gradients have opposite signs → return 0 (extremum detection)
     * - If gradients have same sign → return limited value based on scheme
     * 
     * Typical usage in MUSCL reconstruction:
     * ```cpp
     * real limited_slope = limiter->limit(dq_ij, dq_local);
     * real reconstructed_value = value - limited_slope * delta;
     * ```
     */
    virtual real limit(real upstream_gradient, real local_gradient) const = 0;
    
    /**
     * @brief Get the name of the limiter scheme
     * @return Human-readable name (e.g., "VanLeer", "MinMod", "Superbee")
     */
    virtual std::string name() const = 0;
};

} // namespace limiters
} // namespace algorithms
} // namespace sph

#endif // SPH_ALGORITHMS_LIMITERS_SLOPE_LIMITER_HPP

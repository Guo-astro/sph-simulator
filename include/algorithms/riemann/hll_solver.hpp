#ifndef SPH_ALGORITHMS_RIEMANN_HLL_SOLVER_HPP
#define SPH_ALGORITHMS_RIEMANN_HLL_SOLVER_HPP

#include "riemann_solver.hpp"
#include "utilities/constants.hpp"

namespace sph {
namespace algorithms {
namespace riemann {

/**
 * @brief HLL (Harten-Lax-van Leer) Riemann solver
 * 
 * Two-wave approximation of the Riemann problem using Roe-averaged wave speeds.
 * Provides robust, efficient solution but is diffusive at contact discontinuities.
 * 
 * Algorithm outline:
 * 1. Compute Roe-averaged sound speed and velocity:
 *    c̃ = (√ρ_L c_L + √ρ_R c_R) / (√ρ_L + √ρ_R)
 *    ũ = (√ρ_L u_L + √ρ_R u_R) / (√ρ_L + √ρ_R)
 * 
 * 2. Estimate left and right wave speeds:
 *    S_L = min(u_L - c_L, ũ - c̃)
 *    S_R = max(u_R + c_R, ũ + c̃)
 * 
 * 3. Compute interface state from jump conditions:
 *    Uses conservation of mass and momentum across the waves
 * 
 * Properties:
 * - Positivity preserving: P* > 0 if P_L, P_R > 0
 * - Entropy satisfying: Respects second law of thermodynamics
 * - Fast: O(1) computational cost
 * - Diffusive: Smears contact discontinuities
 * 
 * Reference: Toro (2009), Section 10.3
 */
class HLLSolver : public RiemannSolver {
public:
    /**
     * @brief Solve Riemann problem using HLL approximation
     * 
     * @param left_state Left thermodynamic state
     * @param right_state Right thermodynamic state
     * @return Interface state (P*, u*)
     */
    RiemannSolution solve(const RiemannState& left_state, 
                         const RiemannState& right_state) const override;
    
    /**
     * @brief Get solver name
     * @return "HLL"
     */
    std::string get_name() const override { return "HLL"; }

private:
    /**
     * @brief Roe-averaged quantities for wave speed estimation
     * 
     * Uses density-weighted averaging to compute intermediate values:
     * q̃ = (√ρ_L q_L + √ρ_R q_R) / (√ρ_L + √ρ_R)
     */
    struct RoeAverages {
        real velocity;      ///< ũ: Roe-averaged velocity
        real sound_speed;   ///< c̃: Roe-averaged sound speed
    };
    
    /**
     * @brief Compute Roe-averaged velocity and sound speed
     * 
     * Roe averaging provides better wave speed estimates than arithmetic averaging,
     * especially for strong shocks and large density ratios.
     * 
     * @param left_state Left thermodynamic state
     * @param right_state Right thermodynamic state
     * @return Roe-averaged velocity and sound speed
     */
    RoeAverages compute_roe_averages(const RiemannState& left_state,
                                     const RiemannState& right_state) const;
};

} // namespace riemann
} // namespace algorithms
} // namespace sph

#endif // SPH_ALGORITHMS_RIEMANN_HLL_SOLVER_HPP

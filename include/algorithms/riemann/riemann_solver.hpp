#ifndef SPH_ALGORITHMS_RIEMANN_RIEMANN_SOLVER_HPP
#define SPH_ALGORITHMS_RIEMANN_RIEMANN_SOLVER_HPP

#include "defines.hpp"
#include <string>

namespace sph {
namespace algorithms {
namespace riemann {

/**
 * @brief State vector for 1D Riemann problem
 * 
 * Represents thermodynamic state on left or right side of a discontinuity.
 * Used as input to Riemann solvers for computing interface states.
 */
struct RiemannState {
    real velocity;      ///< u: Velocity in normal direction [m/s]
    real density;       ///< ρ: Mass density [kg/m³]
    real pressure;      ///< P: Pressure [Pa]
    real sound_speed;   ///< c: Sound speed [m/s]
    
    /**
     * @brief Validate that state is physically meaningful
     * @return true if density, pressure, and sound speed are all positive
     */
    bool is_valid() const {
        return (density > 0.0) && (pressure > 0.0) && (sound_speed > 0.0);
    }
};

/**
 * @brief Solution of Riemann problem at interface
 * 
 * Represents the resolved state between left and right regions
 * after solving the Riemann problem.
 */
struct RiemannSolution {
    real pressure;      ///< P*: Interface pressure [Pa]
    real velocity;      ///< u*: Interface velocity [m/s]
    
    /**
     * @brief Validate that solution is physically meaningful
     * @return true if pressure is positive and velocity is finite
     */
    bool is_valid() const {
        return (pressure > 0.0) && std::isfinite(velocity);
    }
};

/**
 * @brief Abstract interface for Riemann solvers
 * 
 * The Riemann problem arises when two fluid states with different properties
 * (density, pressure, velocity) are separated by a discontinuity. The solution
 * determines the interface state (P*, u*) that satisfies conservation laws.
 * 
 * Physical background:
 * - Conservation of mass, momentum, and energy across the interface
 * - Wave structure: shock, rarefaction, and contact discontinuity
 * - Entropy condition: physically admissible solutions
 * 
 * Common solver types:
 * - HLL (Harten-Lax-van Leer): Fast, two-wave approximation, diffusive at contact
 * - HLLC: HLL with Contact wave restoration (less diffusive)
 * - Exact: Iterative exact solver (most accurate, computationally expensive)
 * 
 * All implementations must:
 * - Handle edge cases: vacuum formation, strong shocks, sonic points
 * - Validate input states (positive density, pressure, sound speed)
 * - Return physically valid solutions (positive pressure, finite velocity)
 * - Be independently testable with comprehensive BDD test suites
 * 
 * Reference: Toro (2009) "Riemann Solvers and Numerical Methods for Fluid Dynamics"
 */
class RiemannSolver {
public:
    virtual ~RiemannSolver() = default;
    
    /**
     * @brief Solve Riemann problem for interface state
     * 
     * Given left and right thermodynamic states, compute the interface
     * state (P*, u*) that satisfies conservation laws.
     * 
     * @param left_state Left state (u_L, ρ_L, P_L, c_L)
     * @param right_state Right state (u_R, ρ_R, P_R, c_R)
     * @return Interface state (P*, u*)
     * 
     * @note Implementations should validate inputs and handle edge cases:
     *       - Vacuum formation (states moving apart)
     *       - Strong shocks (extreme pressure ratios)
     *       - Contact discontinuities (pressure equilibrium, density jump)
     *       - Sonic points (transonic flow)
     *       - Extreme density ratios (1e-6 to 1e6)
     */
    virtual RiemannSolution solve(const RiemannState& left_state, 
                                   const RiemannState& right_state) const = 0;
    
    /**
     * @brief Get solver name for logging and debugging
     * @return Human-readable name (e.g., "HLL", "HLLC", "Exact")
     */
    virtual std::string get_name() const = 0;
};

} // namespace riemann
} // namespace algorithms
} // namespace sph

#endif // SPH_ALGORITHMS_RIEMANN_RIEMANN_SOLVER_HPP

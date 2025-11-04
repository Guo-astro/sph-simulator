#include "algorithms/riemann/hll_solver.hpp"
#include "utilities/constants.hpp"
#include <cmath>
#include <algorithm>

namespace sph {
namespace algorithms {
namespace riemann {

using namespace utilities::constants;

HLLSolver::RoeAverages 
HLLSolver::compute_roe_averages(const RiemannState& left_state,
                                const RiemannState& right_state) const 
{
    // Compute square roots of densities for Roe averaging
    const real sqrt_rho_left = std::sqrt(left_state.density);
    const real sqrt_rho_right = std::sqrt(right_state.density);
    const real inv_sum = ONE / (sqrt_rho_left + sqrt_rho_right);
    
    // Roe-averaged quantities using density weighting
    RoeAverages avg;
    avg.velocity = (sqrt_rho_left * left_state.velocity + 
                    sqrt_rho_right * right_state.velocity) * inv_sum;
    avg.sound_speed = (sqrt_rho_left * left_state.sound_speed + 
                       sqrt_rho_right * right_state.sound_speed) * inv_sum;
    
    return avg;
}

RiemannSolution 
HLLSolver::solve(const RiemannState& left_state, 
                 const RiemannState& right_state) const 
{
    // Validate input states
    if (!left_state.is_valid() || !right_state.is_valid()) {
        // Return a safe fallback - this should be handled by the caller
        // but we provide a reasonable default to avoid crashes
        RiemannSolution fallback;
        fallback.pressure = HALF * (left_state.pressure + right_state.pressure);
        fallback.velocity = HALF * (left_state.velocity + right_state.velocity);
        return fallback;
    }
    
    // Compute Roe-averaged wave speeds for better shock capturing
    const auto avg = compute_roe_averages(left_state, right_state);
    
    // Estimate left and right wave speeds using Roe averages
    // This provides better accuracy than using direct left/right values
    const real wave_speed_left = std::min(left_state.velocity - left_state.sound_speed,
                                         avg.velocity - avg.sound_speed);
    const real wave_speed_right = std::max(right_state.velocity + right_state.sound_speed,
                                          avg.velocity + avg.sound_speed);
    
    // Compute intermediate state coefficients from conservation laws
    // These coefficients relate the left/right states to the interface state
    const real coeff_left = left_state.density * (wave_speed_left - left_state.velocity);
    const real coeff_right = right_state.density * (wave_speed_right - right_state.velocity);
    const real coeff_inv = ONE / (coeff_left - coeff_right);
    
    // Compute momentum flux difference across the waves
    const real flux_diff_left = left_state.pressure - left_state.velocity * coeff_left;
    const real flux_diff_right = right_state.pressure - right_state.velocity * coeff_right;
    
    // Interface state from conservation of mass and momentum
    RiemannSolution solution;
    solution.velocity = (flux_diff_right - flux_diff_left) * coeff_inv;
    solution.pressure = (coeff_left * flux_diff_right - 
                        coeff_right * flux_diff_left) * coeff_inv;
    
    return solution;
}

} // namespace riemann
} // namespace algorithms
} // namespace sph

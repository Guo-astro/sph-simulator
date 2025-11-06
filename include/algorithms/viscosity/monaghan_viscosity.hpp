/**
 * @file monaghan_viscosity.hpp
 * @brief Monaghan artificial viscosity with Balsara switch
 * 
 * Implementation of the standard Monaghan (1997) artificial viscosity:
 * 
 * π_ij = -α_ij v_sig w_ij / (2 ρ_ij)    if v_ij · r_ij < 0 (approaching)
 * π_ij = 0                               if v_ij · r_ij ≥ 0 (receding)
 * 
 * where:
 *   α_ij = (α_i + α_j) / 2              (average viscosity coefficient)
 *   v_sig = c_i + c_j - 3 w_ij          (signal velocity, Monaghan 1997)
 *   w_ij = (v_ij · r_ij) / |r_ij|       (relative velocity along line of centers)
 *   ρ_ij = (ρ_i + ρ_j) / 2              (average density)
 * 
 * Optional Balsara switch (Morris & Monaghan 1997):
 * Reduces viscosity in shear flows while maintaining it in compressive flows:
 *   f_i = |∇·v_i| / (|∇·v_i| + |∇×v_i| + ε c_i / h_i)
 *   
 * The switch multiplies the viscosity: π_ij → f_ij π_ij where f_ij = (f_i + f_j)/2
 * 
 * References:
 * - Monaghan (1997): "SPH and Riemann solvers", J. Comp. Phys. 136, 298
 * - Morris & Monaghan (1997): "A switch to reduce SPH viscosity", J. Comp. Phys. 136, 41
 */

#pragma once

#include "artificial_viscosity.hpp"
#include "../../utilities/constants.hpp"
#include "../../core/utilities/vector.hpp"
#include <algorithm>
#include <string>
#include <cmath>

namespace sph {
namespace algorithms {
namespace viscosity {

// Import constants for cleaner code
using namespace sph::utilities::constants;

/**
 * @brief Monaghan (1997) artificial viscosity with optional Balsara switch
 * 
 * Standard SPH artificial viscosity that provides:
 * - Shock capturing through velocity-dependent dissipation
 * - Prevention of particle interpenetration
 * - Optional reduction in shear flows via Balsara switch
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class MonaghanViscosity : public ArtificialViscosity<Dim> {
public:
    /**
     * @brief Construct Monaghan viscosity
     * 
     * @param use_balsara_switch Whether to apply Balsara switch to reduce shear viscosity
     */
    explicit MonaghanViscosity(bool use_balsara_switch = true)
        : m_use_balsara_switch(use_balsara_switch)
    {}
    
    /**
     * @brief Compute Monaghan viscosity between two particles
     * 
     * @param state Particle pair state
     * @return Viscosity term π_ij
     * 
     * Algorithm:
     * 1. Check if particles are approaching (v_ij · r_ij < 0)
     * 2. Compute signal velocity v_sig = c_i + c_j - 3 w_ij
     * 3. Compute relative velocity projection w_ij = (v_ij · r_ij) / r
     * 4. Apply Balsara switch if enabled: f_ij = (f_i + f_j) / 2
     * 5. Return π_ij = -f_ij α_ij v_sig w_ij / (2 ρ_ij)
     */
    real compute(const ViscosityState<Dim>& state) const override {
        const auto& p_i = state.p_i;
        const auto& p_j = state.p_j;
        const auto& r_ij = state.r_ij;
        const real r = state.r;
        
        // Velocity difference
        const Vector<Dim> v_ij = p_i.vel - p_j.vel;
        
        // Relative velocity along line of centers
        const real vr = sph::inner_product(v_ij, r_ij);
        
        // Only apply viscosity for approaching particles
        if (vr >= ZERO) {
            return ZERO;
        }
        
        // Average viscosity coefficient
        const real alpha = HALF * (p_i.alpha + p_j.alpha);
        
        // Balsara switch (if enabled)
        const real balsara = m_use_balsara_switch 
            ? HALF * (p_i.balsara + p_j.balsara)
            : ONE;
        
        // Relative velocity projection: w_ij = (v_ij · r_ij) / r
        const real w_ij = vr / r;
        
        // Signal velocity (Monaghan 1997, eq. 30)
        // v_sig = c_i + c_j - 3 w_ij
        // The factor of 3 comes from analyzing the Riemann problem
        const real v_sig = p_i.sound + p_j.sound - THREE * w_ij;
        
        // Average density (harmonic mean)
        const real rho_ij_inv = TWO / (p_i.dens + p_j.dens);
        
        // Monaghan viscosity: π_ij = -α v_sig w_ij / (2 ρ_ij)
        // The factor of 0.5 comes from averaging over particle pair
        const real pi_ij = -HALF * balsara * alpha * v_sig * w_ij * rho_ij_inv;
        
        return pi_ij;
    }
    
    std::string name() const override {
        return m_use_balsara_switch 
            ? "Monaghan (1997) with Balsara switch"
            : "Monaghan (1997) standard";
    }
    
private:
    bool m_use_balsara_switch;  ///< Whether to use Balsara switch
};

} // namespace viscosity
} // namespace algorithms
} // namespace sph

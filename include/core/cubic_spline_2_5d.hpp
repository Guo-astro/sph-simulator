#pragma once

#include "kernel_function.hpp"
#include <cmath>

namespace sph {
namespace Spline {

/**
 * @brief 2.5D Cubic spline kernel (2D hydro + 3D gravity)
 * 
 * Uses 2D kernel for hydrodynamic calculations but provides
 * 3D normalization for gravity calculations when needed.
 */
class Cubic25D : public KernelFunction<2> {
public:
    Cubic25D() = default;
    
    /**
     * @brief 2D kernel value W_2D(r, h) for hydrodynamics
     */
    real w(const Vector<2>& r, real h) const override {
        real q = abs(r) / h;
        if (q >= 2.0) return 0.0;
        
        real sigma = 10.0 / (7.0 * M_PI * h * h); // 2D normalization
        
        if (q < 1.0) {
            real q2 = q * q;
            real q3 = q2 * q;
            return sigma * (1.0 - 1.5 * q2 + 0.75 * q3);
        } else {
            real q2 = q * q;
            real q3 = q2 * q;
            return sigma * 0.25 * (2.0 - q) * (2.0 - q) * (2.0 - q);
        }
    }
    
    /**
     * @brief 2D kernel gradient dW/dr for hydrodynamics
     */
    Vector<2> dw(const Vector<2>& r, real h) const override {
        real q = abs(r) / h;
        if (q >= 2.0 || q == 0.0) return Vector<2>(0.0);
        
        real sigma = 10.0 / (7.0 * M_PI * h * h * h); // 2D normalization with h^3
        real dW_dq = 0.0;
        
        if (q < 1.0) {
            dW_dq = -3.0 * q + 2.25 * q * q;
        } else {
            dW_dq = -0.75 * (2.0 - q) * (2.0 - q);
        }
        
        return (sigma * dW_dq / (q * h)) * r; // Chain rule: dW/dq * dq/dr * dr
    }
    
    /**
     * @brief Laplacian of kernel for viscosity (2D)
     */
    real dhw(const Vector<2>& r, real h) const override {
        real q = abs(r) / h;
        if (q >= 2.0 || q == 0.0) return 0.0;
        
        real sigma = 10.0 / (7.0 * M_PI * h * h * h * h); // 2D normalization with h^4
        real d2W_dq2 = 0.0;
        
        if (q < 1.0) {
            d2W_dq2 = -3.0 + 4.5 * q;
        } else {
            d2W_dq2 = 1.5 * (2.0 - q);
        }
        
        return sigma * d2W_dq2 / (h * h);
    }
    
    /**
     * @brief Get 3D normalization constant for gravity calculations
     * Useful when comparing with full 3D gravity
     */
    static constexpr real sigma_3d() {
        return 1.0 / M_PI;
    }
    
    /**
     * @brief Convert 2D smoothing length to effective 3D smoothing length
     * For gravity calculations that need 3D context
     */
    static real h_2d_to_3d(real h_2d, real particle_mass, real surface_density) {
        // Approximate 3D smoothing length based on surface density
        // h_3d ≈ h_2d * sqrt(Σ / ρ_3d) where Σ is surface density
        // This is approximate and may need calibration
        return h_2d * 2.0; // Conservative estimate
    }
};

} // namespace Spline
} // namespace sph</content>
<parameter name="filePath">/Users/guo/sph-simulation/include/core/cubic_spline_2_5d.hpp
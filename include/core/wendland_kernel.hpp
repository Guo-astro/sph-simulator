/**
 * @file wendland_kernel.hpp
 * @brief Template-based Wendland C4 kernel
 * 
 * Refactored from DIM macro to templates.
 * Reference: Wendland (1995), Dehnen & Aly (2012)
 */

#pragma once

#include "../core/kernel_function.hpp"
#include "../defines.hpp"
#include <cmath>
#include <stdexcept>

namespace sph {
namespace Wendland {

/**
 * @brief Get normalization constant for Wendland C4 kernel
 * @tparam Dim Spatial dimension
 * @note Not defined for 1D (returns 0)
 */
template<int Dim>
inline constexpr real sigma_c4() {
    if constexpr(Dim == 1) {
        return 0.0;  // Not defined for 1D
    } else if constexpr(Dim == 2) {
        return 9.0 / M_PI;
    } else if constexpr(Dim == 3) {
        return 495.0 / (32.0 * M_PI);
    }
}

/**
 * @brief Wendland C4 kernel function
 * @tparam Dim Spatial dimension (2 or 3 only)
 * 
 * W(q) = σ * (1-q)⁶₊ * (1 + 6q + 35q²/3)
 * where q = r/h and (·)₊ means max(0, ·)
 * 
 * Note: This kernel is not defined for 1D simulations
 */
template<int Dim>
class C4Kernel : public KernelFunction<Dim> {
public:
    C4Kernel() {
        static_assert(Dim >= 2, "Wendland C4 kernel requires dimension >= 2");
    }
    
    /**
     * @brief Kernel value W(r, h)
     */
    real w(real r, real h) const override {
        const real q = r / h;
        const real sigma = sigma_c4<Dim>();
        
        return sigma / powh<Dim>(h) *
            pow6(0.5 * (1.0 - q + std::abs(1.0 - q))) *
            (1.0 + 6.0 * q + 35.0 / 3.0 * q * q);
    }
    
    /**
     * @brief Gradient ∇W(r, h)
     */
    Vector<Dim> dw(const Vector<Dim>& rij, real r, real h) const override {
        const real q = r / h;
        const real sigma = sigma_c4<Dim>();
        
        const real c = -56.0 / 3.0 * sigma / (powh<Dim>(h) * sqr(h)) *
            pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
            (1.0 + 5.0 * q);
        
        return rij * c;
    }
    
    /**
     * @brief Derivative ∂W/∂h
     */
    real dhw(real r, real h) const override {
        const real q = r / h;
        const real sigma = sigma_c4<Dim>();
        
        return -sigma / (powh<Dim>(h) * h * 3.0) *
            pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
            (3.0 * Dim + 15.0 * Dim * q +
             (-56.0 + 17.0 * Dim) * q * q -
             35.0 * (8.0 + Dim) * pow3(q));
    }
};

// Type aliases for convenience (no 1D variant)
using C4Kernel2D = C4Kernel<2>;
using C4Kernel3D = C4Kernel<3>;

} // namespace Wendland
} // namespace sph

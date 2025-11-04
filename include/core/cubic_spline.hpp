/**
 * @file cubic_spline.hpp
 * @brief Template-based cubic spline kernel
 * 
 * Refactored from DIM macro to templates.
 * Reference: Monaghan & Lattanzio (1985)
 */

#pragma once

#include "../core/kernel_function.hpp"
#include "../defines.hpp"
#include <cmath>

namespace sph {
namespace Spline {

/**
 * @brief Get normalization constant for cubic spline kernel
 * @tparam Dim Spatial dimension
 */
template<int Dim>
inline constexpr real sigma_cubic() {
    if constexpr(Dim == 1) {
        return 2.0 / 3.0;
    } else if constexpr(Dim == 2) {
        return 10.0 / (7.0 * M_PI);
    } else if constexpr(Dim == 3) {
        return 1.0 / M_PI;
    }
}

/**
 * @brief Cubic spline kernel function
 * @tparam Dim Spatial dimension (1, 2, or 3)
 * 
 * W(q) = σ * {
 *   (2-q)³/4 - (1-q)³   for 0 ≤ q < 1
 *   (2-q)³/4            for 1 ≤ q < 2
 *   0                   for q ≥ 2
 * }
 * where q = 2r/h
 */
template<int Dim>
class Cubic : public KernelFunction<Dim> {
public:
    Cubic() = default;
    
    /**
     * @brief Kernel value W(r, h)
     */
    real w(real r, real h) const override {
        const real h_ = h * 0.5;
        const real q = r / h_;
        const real sigma = sigma_cubic<Dim>();
        
        return sigma / powh<Dim>(h_) * (
            0.25 * pow3(0.5 * (2.0 - q + std::abs(2.0 - q))) -
            pow3(0.5 * (1.0 - q + std::abs(1.0 - q)))
        );
    }
    
    /**
     * @brief Gradient ∇W(r, h)
     */
    Vector<Dim> dw(const Vector<Dim>& rij, real r, real h) const override {
        if(r == 0.0) {
            Vector<Dim> zero;  // Default constructor creates zero vector
            return zero;
        }
        
        const real h_ = h * 0.5;
        const real q = r / h_;
        const real sigma = sigma_cubic<Dim>();
        
        const real c = -sigma / (powh<Dim>(h_) * h_ * r) * (
            0.75 * sqr(0.5 * (2.0 - q + std::abs(2.0 - q))) -
            3.0 * sqr(0.5 * (1.0 - q + std::abs(1.0 - q)))
        );
        
        return rij * c;
    }
    
    /**
     * @brief Derivative ∂W/∂h
     */
    real dhw(real r, real h) const override {
        const real h_ = h * 0.5;
        const real q = r / h_;
        const real sigma = sigma_cubic<Dim>();
        
        return 0.5 * sigma / (powh<Dim>(h_) * h_) * (
            sqr((std::abs(2.0 - q) + 2.0 - q) * 0.5) * ((3.0 + Dim) * 0.25 * q - 0.5 * Dim) +
            sqr((std::abs(1.0 - q) + 1.0 - q) * 0.5) * ((-3.0 - Dim) * q + Dim)
        );
    }
};

// Type aliases for convenience
using Cubic1D = Cubic<1>;
using Cubic2D = Cubic<2>;
using Cubic3D = Cubic<3>;

} // namespace Spline
} // namespace sph

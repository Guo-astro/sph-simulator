/**
 * @file kernel_function.hpp
 * @brief Template-based dimension-agnostic kernel function interface
 * 
 * Refactored from preprocessor-based DIM macro to templates.
 * Benefits:
 * - Type-safe compile-time dimension checking
 * - No preprocessor conditionals
 * - Testable in single binary
 */

#pragma once

#include "../utilities/vector.hpp"
#include <cmath>

namespace sph {

/**
 * @brief Compute h raised to the power of dimension
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
inline constexpr real powh(real h) {
    if constexpr(Dim == 1) {
        return h;
    } else if constexpr(Dim == 2) {
        return h * h;
    } else if constexpr(Dim == 3) {
        return h * h * h;
    }
}

/**
 * @brief Abstract base class for SPH kernel functions
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class KernelFunction {
public:
    virtual ~KernelFunction() = default;
    
    /**
     * @brief Kernel function W(r, h)
     * @param r Distance between particles
     * @param h Smoothing length
     * @return Kernel value
     */
    virtual real w(real r, real h) const = 0;
    
    /**
     * @brief Gradient of kernel function ∇W(r, h)
     * @param rij Vector from particle j to particle i
     * @param r Distance |rij|
     * @param h Smoothing length
     * @return Gradient vector
     */
    virtual Vector<Dim> dw(const Vector<Dim>& rij, real r, real h) const = 0;
    
    /**
     * @brief Derivative of kernel with respect to smoothing length ∂W/∂h
     * @param r Distance between particles
     * @param h Smoothing length
     * @return Derivative value
     */
    virtual real dhw(real r, real h) const = 0;
    
    /**
     * @brief Get the dimension of this kernel
     */
    static constexpr int dimension() { return Dim; }
};

} // namespace sph

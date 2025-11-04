/**
 * @file vector.hpp
 * @brief Template-based dimension-agnostic vector class
 * 
 * Replaces the old vec_t with DIM macro approach.
 * Benefits:
 * - No preprocessor conditionals
 * - Compile-time type safety
 * - Support for runtime dimension selection via std::variant
 * - Clean, maintainable code
 * - Fully testable in single binary
 */

#pragma once

#include "../defines.hpp"
#include <cmath>
#include <type_traits>
#include <array>

namespace sph {

/**
 * @brief Dimension-agnostic vector class using templates
 * @tparam Dim Number of dimensions (1, 2, or 3)
 */
template<int Dim>
class Vector {
    static_assert(Dim >= 1 && Dim <= 3, "Vector dimension must be 1, 2, or 3");
    
    real data[Dim];
    
public:
    static constexpr int dimension = Dim;
    
    // ============================================================
    // Constructors
    // ============================================================
    
    /**
     * @brief Default constructor - initializes to zero
     */
    Vector() {
        for(int i = 0; i < Dim; ++i) {
            data[i] = 0.0;
        }
    }
    
    /**
     * @brief Constructor for 1D vectors
     */
    template<int D = Dim, typename = typename std::enable_if<D == 1>::type>
    explicit Vector(real x) {
        data[0] = x;
    }
    
    /**
     * @brief Constructor for 2D vectors
     */
    template<int D = Dim, typename = typename std::enable_if<D == 2>::type>
    Vector(real x, real y) {
        data[0] = x;
        data[1] = y;
    }
    
    /**
     * @brief Constructor for 3D vectors
     */
    template<int D = Dim, typename = typename std::enable_if<D == 3>::type>
    Vector(real x, real y, real z) {
        data[0] = x;
        data[1] = y;
        data[2] = z;
    }
    
    /**
     * @brief Copy constructor
     */
    Vector(const Vector& other) {
        for(int i = 0; i < Dim; ++i) {
            data[i] = other.data[i];
        }
    }
    
    /**
     * @brief Construct from C array
     */
    explicit Vector(const real (&arr)[Dim]) {
        for(int i = 0; i < Dim; ++i) {
            data[i] = arr[i];
        }
    }
    
    // ============================================================
    // Element access
    // ============================================================
    
    real& operator[](int i) {
        return data[i];
    }
    
    const real& operator[](int i) const {
        return data[i];
    }
    
    const real* get_array() const {
        return data;
    }
    
    // ============================================================
    // Assignment operators
    // ============================================================
    
    Vector& operator=(const Vector& other) {
        if(this != &other) {
            for(int i = 0; i < Dim; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }
    
    Vector& operator=(const real (&arr)[Dim]) {
        for(int i = 0; i < Dim; ++i) {
            data[i] = arr[i];
        }
        return *this;
    }
    
    Vector& operator=(real scalar) {
        for(int i = 0; i < Dim; ++i) {
            data[i] = scalar;
        }
        return *this;
    }
    
    // ============================================================
    // Unary operators
    // ============================================================
    
    const Vector& operator+() const {
        return *this;
    }
    
    Vector operator-() const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = -data[i];
        }
        return result;
    }
    
    // ============================================================
    // Compound assignment operators
    // ============================================================
    
    Vector& operator+=(const Vector& other) {
        for(int i = 0; i < Dim; ++i) {
            data[i] += other.data[i];
        }
        return *this;
    }
    
    Vector& operator+=(const real (&arr)[Dim]) {
        for(int i = 0; i < Dim; ++i) {
            data[i] += arr[i];
        }
        return *this;
    }
    
    Vector& operator+=(real scalar) {
        for(int i = 0; i < Dim; ++i) {
            data[i] += scalar;
        }
        return *this;
    }
    
    Vector& operator-=(const Vector& other) {
        for(int i = 0; i < Dim; ++i) {
            data[i] -= other.data[i];
        }
        return *this;
    }
    
    Vector& operator-=(const real (&arr)[Dim]) {
        for(int i = 0; i < Dim; ++i) {
            data[i] -= arr[i];
        }
        return *this;
    }
    
    Vector& operator-=(real scalar) {
        for(int i = 0; i < Dim; ++i) {
            data[i] -= scalar;
        }
        return *this;
    }
    
    Vector& operator*=(real scalar) {
        for(int i = 0; i < Dim; ++i) {
            data[i] *= scalar;
        }
        return *this;
    }
    
    Vector& operator/=(real scalar) {
        for(int i = 0; i < Dim; ++i) {
            data[i] /= scalar;
        }
        return *this;
    }
    
    // ============================================================
    // Binary arithmetic operators
    // ============================================================
    
    Vector operator+(const Vector& other) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] + other.data[i];
        }
        return result;
    }
    
    Vector operator+(const real (&arr)[Dim]) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] + arr[i];
        }
        return result;
    }
    
    Vector operator+(real scalar) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] + scalar;
        }
        return result;
    }
    
    Vector operator-(const Vector& other) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] - other.data[i];
        }
        return result;
    }
    
    Vector operator-(const real (&arr)[Dim]) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] - arr[i];
        }
        return result;
    }
    
    Vector operator-(real scalar) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] - scalar;
        }
        return result;
    }
    
    Vector operator*(real scalar) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] * scalar;
        }
        return result;
    }
    
    Vector operator/(real scalar) const {
        Vector result;
        for(int i = 0; i < Dim; ++i) {
            result.data[i] = data[i] / scalar;
        }
        return result;
    }
};

// ============================================================
// Type aliases for convenience
// ============================================================

using Vector1D = Vector<1>;
using Vector2D = Vector<2>;
using Vector3D = Vector<3>;

// ============================================================
// Free functions
// ============================================================

/**
 * @brief Inner product (dot product) of two vectors
 */
template<int Dim>
inline real inner_product(const Vector<Dim>& a, const Vector<Dim>& b) {
    real result = 0.0;
    for(int i = 0; i < Dim; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

/**
 * @brief Inner product with C array
 */
template<int Dim>
inline real inner_product(const Vector<Dim>& a, const real (&b)[Dim]) {
    real result = 0.0;
    for(int i = 0; i < Dim; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

/**
 * @brief Inner product of two C arrays
 */
template<int Dim>
inline real inner_product(const real (&a)[Dim], const real (&b)[Dim]) {
    real result = 0.0;
    for(int i = 0; i < Dim; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

/**
 * @brief Squared magnitude of a vector
 */
template<int Dim>
inline real abs2(const Vector<Dim>& v) {
    return inner_product(v, v);
}

/**
 * @brief Magnitude of a vector
 */
template<int Dim>
inline real abs(const Vector<Dim>& v) {
    return std::sqrt(abs2(v));
}

/**
 * @brief Distance between two vectors
 */
template<int Dim>
inline real distance(const Vector<Dim>& a, const Vector<Dim>& b) {
    return abs(a - b);
}

/**
 * @brief Scalar multiplication (scalar * vector)
 */
template<int Dim>
inline Vector<Dim> operator*(real scalar, const Vector<Dim>& v) {
    return v * scalar;
}

/**
 * @brief 2D vector product (returns z-component of cross product)
 * Only available for 2D vectors
 */
inline real vector_product(const Vector2D& a, const Vector2D& b) {
    return a[0] * b[1] - a[1] * b[0];
}

/**
 * @brief 3D cross product
 * Only available for 3D vectors
 */
inline Vector3D cross_product(const Vector3D& a, const Vector3D& b) {
    return Vector3D(
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    );
}

} // namespace sph

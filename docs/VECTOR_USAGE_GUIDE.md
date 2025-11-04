# Using the New Dimension-Agnostic Vector System

## Quick Start

### Include the Header
```cpp
#include "core/vector.hpp"

using namespace sph;
```

### Basic Usage

#### 1D Vectors
```cpp
// Construction
Vector1D v1(5.0);
Vector1D v2(3.0);

// Operations
Vector1D sum = v1 + v2;        // 8.0
Vector1D diff = v1 - v2;       // 2.0
Vector1D scaled = v1 * 2.0;    // 10.0
real dot = inner_product(v1, v2);  // 15.0
```

#### 2D Vectors
```cpp
// Construction
Vector2D v1(1.0, 2.0);
Vector2D v2(3.0, 4.0);

// Operations
Vector2D sum = v1 + v2;           // (4.0, 6.0)
real dot = inner_product(v1, v2);  // 11.0
real mag = abs(v1);                // 2.236...
real cross_z = vector_product(v1, v2);  // -2.0
```

#### 3D Vectors
```cpp
// Construction
Vector3D v1(1.0, 2.0, 3.0);
Vector3D v2(4.0, 5.0, 6.0);

// Operations
Vector3D sum = v1 + v2;            // (5.0, 7.0, 9.0)
real dot = inner_product(v1, v2);   // 32.0
Vector3D cross = cross_product(v1, v2);  // (-3.0, 6.0, -3.0)
```

## Migration from Old DIM Macro System

### Before (Old Way - BAD ❌)
```cpp
#define DIM 2

vec_t calculate_force(const vec_t& pos1, const vec_t& pos2) {
#if DIM == 1
    vec_t r = vec_t(pos1[0] - pos2[0]);
#elif DIM == 2
    vec_t r = vec_t(pos1[0] - pos2[0], pos1[1] - pos2[1]);
#elif DIM == 3
    vec_t r = vec_t(pos1[0] - pos2[0], pos1[1] - pos2[1], pos1[2] - pos2[2]);
#endif
    return r;
}
```

### After (New Way - GOOD ✅)
```cpp
template<int Dim>
Vector<Dim> calculate_force(const Vector<Dim>& pos1, const Vector<Dim>& pos2) {
    return pos1 - pos2;  // Works for any dimension!
}

// Or use auto for simplicity
auto calculate_force(const auto& pos1, const auto& pos2) {
    return pos1 - pos2;
}
```

## Advanced Patterns

### Generic Algorithm
```cpp
// Works for 1D, 2D, and 3D automatically
template<int Dim>
real compute_distance_squared(const Vector<Dim>& a, const Vector<Dim>& b) {
    Vector<Dim> diff = a - b;
    return abs2(diff);
}
```

### Dimension-Specific Code with `if constexpr`
```cpp
template<int Dim>
void process_vector(const Vector<Dim>& v) {
    if constexpr(Dim == 2) {
        // 2D-specific code
        std::cout << "2D vector with magnitude: " << abs(v) << std::endl;
    } else if constexpr(Dim == 3) {
        // 3D-specific code
        std::cout << "3D vector" << std::endl;
    }
}
```

### Runtime Dimension Selection
```cpp
#include <variant>

using SimVariant = std::variant<
    Simulation<1>,
    Simulation<2>,
    Simulation<3>
>;

SimVariant create_simulation(int dim) {
    switch(dim) {
        case 1: return Simulation<1>();
        case 2: return Simulation<2>();
        case 3: return Simulation<3>();
        default: throw std::invalid_argument("Invalid dimension");
    }
}

// Use with std::visit
void run_simulation(SimVariant& sim) {
    std::visit([](auto& s) { s.step(); }, sim);
}
```

## Common Patterns

### Particle Initialization
```cpp
template<int Dim>
struct Particle {
    Vector<Dim> pos;
    Vector<Dim> vel;
    Vector<Dim> acc;
    real mass;
};

// Initialize particles for 2D simulation
std::vector<Particle<2>> particles;
particles.push_back({
    .pos = Vector2D(0.5, 0.5),
    .vel = Vector2D(0.0, 0.0),
    .acc = Vector2D(0.0, 0.0),
    .mass = 1.0
});
```

### Kernel Function Example
```cpp
template<int Dim>
struct CubicSplineKernel {
    static constexpr real sigma() {
        if constexpr(Dim == 1) return 2.0 / 3.0;
        else if constexpr(Dim == 2) return 10.0 / (7.0 * M_PI);
        else if constexpr(Dim == 3) return 1.0 / M_PI;
    }
    
    real operator()(real q, real h) const {
        real norm_factor = sigma() / std::pow(h, Dim);
        // ... kernel calculation
    }
};
```

### Force Calculation
```cpp
template<int Dim>
Vector<Dim> fluid_force(
    const Vector<Dim>& r_ij,
    real rho_i, real rho_j,
    real p_i, real p_j,
    real m_j)
{
    real r = abs(r_ij);
    real kernel_grad = compute_kernel_gradient(r);
    
    real pressure_term = (p_i / (rho_i * rho_i) + p_j / (rho_j * rho_j));
    
    return r_ij * (-m_j * pressure_term * kernel_grad / r);
}
```

## Testing Your Code

### Unit Test Example
```cpp
TEST(MySimulation, ComputesForceCorrectly_2D) {
    Vector2D r(1.0, 0.0);
    real rho_i = 1.0, rho_j = 1.0;
    real p_i = 1.0, p_j = 1.0;
    real m_j = 1.0;
    
    auto force = fluid_force(r, rho_i, rho_j, p_i, p_j, m_j);
    
    EXPECT_GT(abs(force), 0.0);
}

TEST(MySimulation, Works_AllDimensions) {
    // Test 1D
    auto f1d = fluid_force(Vector1D(1.0), 1.0, 1.0, 1.0, 1.0, 1.0);
    EXPECT_GT(abs(f1d), 0.0);
    
    // Test 2D
    auto f2d = fluid_force(Vector2D(1.0, 0.0), 1.0, 1.0, 1.0, 1.0, 1.0);
    EXPECT_GT(abs(f2d), 0.0);
    
    // Test 3D
    auto f3d = fluid_force(Vector3D(1.0, 0.0, 0.0), 1.0, 1.0, 1.0, 1.0, 1.0);
    EXPECT_GT(abs(f3d), 0.0);
}
```

## Performance Tips

1. **Use references** for function parameters to avoid copies
   ```cpp
   void process(const Vector2D& v);  // Good
   void process(Vector2D v);         // Bad - copies
   ```

2. **Mark functions as inline** for small operations
   ```cpp
   inline Vector2D normalize(const Vector2D& v) {
       return v / abs(v);
   }
   ```

3. **Use constexpr** where possible
   ```cpp
   constexpr int dim = Vector2D::dimension;  // Compile-time constant
   ```

4. **Explicit template instantiation** to reduce compilation time
   ```cpp
   // In a .cpp file
   template class Vector<1>;
   template class Vector<2>;
   template class Vector<3>;
   ```

## Compile-Time Checks

The new system provides compile-time safety:

```cpp
// This won't compile (dimension mismatch)
Vector2D v2d;
Vector3D v3d;
auto sum = v2d + v3d;  // ERROR: template type mismatch

// This won't compile (invalid dimension)
Vector<4> v4d;  // ERROR: static_assert fails

// This won't compile (wrong constructor)
Vector2D v(1.0, 2.0, 3.0);  // ERROR: no matching constructor
```

## FAQ

### Q: Is there any runtime performance penalty?
**A:** No! Templates are resolved at compile time. The generated machine code is identical to the macro-based approach.

### Q: Can I still use the old `vec_t` type?
**A:** Yes, during the migration period both coexist. However, new code should use `Vector<Dim>`.

### Q: How do I choose the dimension at runtime?
**A:** Use `std::variant` as shown in the "Runtime Dimension Selection" section above.

### Q: Will this increase compile time?
**A:** Slightly, but the benefits far outweigh the cost. Use explicit instantiation if it becomes an issue.

### Q: Can I add 4D support?
**A:** Yes! Simply change the static_assert and add the constructors. The rest works automatically.

## Summary

The new template-based system provides:
- ✅ Type safety
- ✅ Better IDE support
- ✅ Easier testing
- ✅ No code duplication
- ✅ Same performance as macros
- ✅ Better maintainability
- ✅ Extensibility

Migrate your code gradually and enjoy the benefits of modern C++!

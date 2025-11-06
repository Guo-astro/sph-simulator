# Migration Guide: Type-Safe Boundary Configuration API

## Overview

The SPH simulation framework now provides a **type-safe, declarative API** for boundary configuration that eliminates error-prone boolean parameters.

## The Problem with Old API

### ❌ Error-Prone Boolean Trap

```cpp
// OLD API - CONFUSING!
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true,       // What does this mean?
    range_min,
    range_max,
    true        // And this? Easy to forget or swap!
);
```

**Problems:**
- Parameter meaning unclear without comments
- Easy to swap boolean parameters
- Easy to forget the ghost particle requirement
- Not self-documenting
- No compile-time safety

### Root Cause of Bugs

The boolean trap led to the catastrophic density explosion bug in the 1D baseline:
- Developer set `enable_ghosts = false` thinking it was optional
- Barnes-Hut tree failed to find periodic neighbors
- Density calculation exploded (3.38×10⁸ instead of ~1.0)
- Simulation crashed

## The Solution: Type-Safe Builder

### ✅ Self-Documenting Declarative API

```cpp
// NEW API - CRYSTAL CLEAR!
auto config = BoundaryBuilder<Dim>()
    .with_periodic_boundaries()    // Intent obvious
    .in_range(range_min, range_max)
    .build();                      // Ghosts automatically enabled!
```

**Benefits:**
- ✅ Intent is crystal clear from method names
- ✅ Ghost particles automatically enabled for all boundary types
- ✅ Impossible to forget architectural requirements
- ✅ Compile-time type safety
- ✅ Fluent, chainable API
- ✅ Self-documenting code

## Migration Examples

### Example 1: Periodic Boundaries (1D Shock Tube)

#### Before (Old API)
```cpp
#include "core/boundary_config_helper.hpp"

Vector<Dim> range_min{-0.5};
Vector<Dim> range_max{1.5};

// Easy to mess up these boolean parameters!
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(
    true,  // periodic = true
    range_min,
    range_max,
    true   // enable_ghosts = ???
);
```

#### After (New API)
```cpp
#include "core/boundary_builder.hpp"

auto config = BoundaryBuilder<Dim>()
    .with_periodic_boundaries()
    .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
    .build();
```

**Difference:**
- 6 lines → 3 lines
- Intent obvious
- No boolean confusion
- Ghosts automatically enabled

### Example 2: Mirror Boundaries (3D Shock Tube)

#### Before (Old API)
```cpp
BoundaryConfiguration<Dim> config;
config.is_valid = true;  // Did you remember this?

// X-direction
config.types[0] = BoundaryType::MIRROR;
config.range_min[0] = -0.5;
config.range_max[0] = 1.5;
config.enable_lower[0] = true;
config.enable_upper[0] = true;
config.mirror_types[0] = MirrorType::FREE_SLIP;
config.spacing_lower[0] = dx_left;
config.spacing_upper[0] = dx_right;

// Y-direction
config.types[1] = BoundaryType::MIRROR;
config.range_min[1] = 0.0;
config.range_max[1] = 0.5;
// ... 6 more lines ...

// Z-direction
// ... 8 more lines ...
```

#### After (New API)
```cpp
auto config = BoundaryBuilder<Dim>()
    .in_range(
        Vector<Dim>{-0.5, 0.0, 0.0},
        Vector<Dim>{1.5, 0.5, 0.5}
    )
    .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)
    .with_mirror_in_dimension(1, MirrorType::NO_SLIP, dy, dy)
    .with_periodic_in_dimension(2)
    .build();
```

**Difference:**
- 30+ lines → 8 lines
- No repetitive boilerplate
- Intent crystal clear
- Less error-prone

### Example 3: Mixed Boundaries

#### Before (Old API)
```cpp
BoundaryConfiguration<Dim> config;
config.is_valid = true;
config.range_min = Vector<Dim>{0.0, 0.0};
config.range_max = Vector<Dim>{1.0, 1.0};

// X: Periodic
config.types[0] = BoundaryType::PERIODIC;
config.enable_lower[0] = true;
config.enable_upper[0] = true;

// Y: Mirror (floor only)
config.types[1] = BoundaryType::MIRROR;
config.enable_lower[1] = true;
config.enable_upper[1] = false;  // Easy to miss!
config.mirror_types[1] = MirrorType::NO_SLIP;
config.spacing_lower[1] = dy;
```

#### After (New API)
```cpp
auto config = BoundaryBuilder<Dim>()
    .in_range(Vector<Dim>{0.0, 0.0}, Vector<Dim>{1.0, 1.0})
    .with_periodic_in_dimension(0)
    .with_mirror_in_dimension(1, MirrorType::NO_SLIP, dy, dy)
    .disable_upper_boundary_in_dimension(1)  // Explicit!
    .build();
```

**Difference:**
- Selective enabling is explicit
- Intent obvious: "floor but no ceiling"
- No easy-to-miss boolean flags

## Static Factory Methods

For simple cases, use convenience factories:

```cpp
// Periodic in all dimensions
auto config = BoundaryBuilder<Dim>::create_periodic(min, max);

// Mirror in all dimensions
auto config = BoundaryBuilder<Dim>::create_mirror(
    min, max, 
    MirrorType::FREE_SLIP, 
    dx  // uniform spacing
);

// No boundaries
auto config = BoundaryBuilder<Dim>::create_none();
```

## Backwards Compatibility

The old `BoundaryConfigHelper` is **deprecated but still available**:

```cpp
// DEPRECATED - Still works but discouraged
#include "core/boundary_config_helper.hpp"
auto config = BoundaryConfigHelper<Dim>::from_baseline_json(...);
```

**Migration Timeline:**
1. **Phase 1 (Current)**: Both APIs available, new API recommended
2. **Phase 2 (Future)**: Old API marked [[deprecated]] with compiler warnings
3. **Phase 3 (TBD)**: Old API removed

## Validation and Error Handling

The new API validates at build time:

```cpp
// COMPILE ERROR - Wrong dimension
Vector<3> wrong_min{0, 0, 0};  // 3D vector
BoundaryBuilder<2>()           // 2D builder
    .in_range(wrong_min, ...)  // Won't compile!
```

And at runtime:

```cpp
// RUNTIME ERROR - Clear message
try {
    BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .build();  // Forgot .in_range()!
} catch (const std::invalid_argument& e) {
    // "BoundaryBuilder: range must be set before building"
}
```

## Testing

The new API is tested with BDD-style tests:

```cpp
TEST(BoundaryBuilder, GivenPeriodicDomain_WhenBuilding_ThenGhostsAreAutomaticallyEnabled) {
    // GIVEN: A 1D domain
    constexpr int Dim = 1;
    
    // WHEN: Building periodic configuration
    auto config = BoundaryBuilder<Dim>()
        .with_periodic_boundaries()
        .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
        .build();
    
    // THEN: Ghost particles are automatically enabled
    EXPECT_TRUE(config.is_valid);
    EXPECT_EQ(config.types[0], BoundaryType::PERIODIC);
}
```

See `/Users/guo/sph-simulation/tests/core/test_boundary_builder.cpp` for comprehensive tests.

## Best Practices

### ✅ DO: Use New API

```cpp
auto config = BoundaryBuilder<Dim>()
    .with_periodic_boundaries()
    .in_range(min, max)
    .build();
```

### ✅ DO: Chain Methods for Readability

```cpp
auto config = BoundaryBuilder<3>()
    .in_range(min, max)
    .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx, dx)
    .with_periodic_in_dimension(1)
    .with_periodic_in_dimension(2)
    .build();
```

### ✅ DO: Use Descriptive Method Names

```cpp
builder.with_periodic_boundaries()  // Clear!
builder.disable_upper_boundary_in_dimension(1)  // Explicit!
```

### ❌ DON'T: Use Old Boolean API

```cpp
// AVOID!
BoundaryConfigHelper::from_baseline_json(true, min, max, true);
```

### ❌ DON'T: Manually Set config Fields

```cpp
// AVOID!
BoundaryConfiguration<Dim> config;
config.is_valid = true;  // Use builder instead!
config.types[0] = ...
```

## FAQ

**Q: Why do ghost particles matter?**
A: Barnes-Hut tree spatial partitioning cannot efficiently find neighbors across periodic boundaries without explicit particle duplication. Ghost particles solve this architectural limitation.

**Q: Are ghosts always enabled?**
A: Yes, for periodic and mirror boundaries. Only open boundaries (`.with_no_boundaries()`) disable ghosts.

**Q: Can I partially enable boundaries?**
A: Yes! Use `.disable_lower_boundary_in_dimension()` or `.disable_upper_boundary_in_dimension()`.

**Q: Is the old API going away?**
A: Eventually, but not soon. You have time to migrate.

**Q: Does this affect performance?**
A: No. The builder just constructs the configuration - runtime performance is identical.

**Q: Can I mix boundary types?**
A: Absolutely! Each dimension can have independent boundary types.

```cpp
auto config = BoundaryBuilder<3>()
    .with_mirror_in_dimension(0, ...)     // X: walls
    .with_periodic_in_dimension(1)        // Y: periodic
    .with_no_boundary_in_dimension(2)     // Z: open
    .in_range(min, max)
    .build();
```

## References

- **Implementation**: `/Users/guo/sph-simulation/include/core/boundary_builder.hpp`
- **Tests**: `/Users/guo/sph-simulation/tests/core/test_boundary_builder.cpp`
- **Example Usage**: `/Users/guo/sph-simulation/workflows/shock_tube_workflow/01_simulation/src/plugin_baseline.cpp`
- **Architecture**: `/Users/guo/sph-simulation/docs/ARCHITECTURE_BOUNDARY_CONSISTENCY.md`
- **Bug Fix**: `/Users/guo/sph-simulation/docs/PERIODIC_BOUNDARY_FIX.md`

---

**Date**: January 2025  
**Status**: ✅ New API ready for production use  
**Migration**: Recommended for all new code

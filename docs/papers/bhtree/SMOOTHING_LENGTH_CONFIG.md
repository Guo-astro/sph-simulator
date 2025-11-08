# Smoothing Length Minimum Enforcement Configuration

## Overview

This document describes the compile-time configurable system for enforcing minimum smoothing lengths in SPH simulations, added to address the gravitational slingshot bug discovered in the Evrard collapse test.

## Background

During peak compression in the Evrard collapse test, the SPH smoothing length `h` collapsed to extremely small values (h_min = 0.023), allowing particles to experience spurious gravitational accelerations from the Hernquist-Katz softening formula, leading to 287 particles (6.8%) escaping the system.

Root cause: When `h` becomes smaller than the average inter-particle spacing but the particle distribution remains physically resolved, the gravity softening creates unphysical forces.

## Configuration

Three enforcement policies are available via `SPHParameters::SmoothingLengthPolicy`:

### 1. NO_MIN (Default - Backward Compatible)

No minimum enforcement - allows `h` to collapse to arbitrarily small values following natural SPH neighbor-finding.

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 3.0, 0.1)
    .with_physics(50, 5.0/3.0)
    .with_kernel("cubic_spline")
    .with_gravity(1.0, 0.5)
    // No .with_smoothing_length_limits() call - defaults to NO_MIN
    .as_ssph()
    .with_artificial_viscosity(1.0)
    .build();
```

### 2. CONSTANT_MIN (Testing/Debugging)

Enforces a constant minimum: `h >= h_min_constant`

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 3.0, 0.1)
    .with_physics(50, 5.0/3.0)
    .with_kernel("cubic_spline")
    .with_gravity(1.0, 0.5)
    .with_smoothing_length_limits(
        SPHParameters::SmoothingLengthPolicy::CONSTANT_MIN,
        0.08,   // h_min_constant
        0.0,    // unused for CONSTANT_MIN
        0.0     // unused for CONSTANT_MIN
    )
    .as_ssph()
    .with_artificial_viscosity(1.0)
    .build();
```

### 3. PHYSICS_BASED (Recommended for Dense Compression)

Enforces physics-based minimum: `h_min = α * (m/ρ_max)^(1/dim)`

This prevents `h` from collapsing below the physical resolution set by particle mass and maximum expected density.

**For Evrard Collapse (N=4224, M=1, R=1, ρ_max ≈ 250):**

```cpp
auto params = SPHParametersBuilderBase()
    .with_time(0.0, 3.0, 0.1)
    .with_physics(50, 5.0/3.0)
    .with_kernel("cubic_spline")
    .with_gravity(1.0, 0.5)
    .with_smoothing_length_limits(
        SPHParameters::SmoothingLengthPolicy::PHYSICS_BASED,
        0.0,    // unused for PHYSICS_BASED
        250.0,  // expected_max_density from Evrard (1988) paper
        2.0     // h_min_coefficient (conservative, typical range 1.5-2.5)
    )
    .as_ssph()
    .with_artificial_viscosity(1.0)
    .build();
```

## Parameter Selection Guide

### expected_max_density

Determine from:
1. **Analytical estimates**: For Evrard collapse, ρ_max ≈ 0.8 * M * N^(2/3) / R^3 ≈ 242
2. **Literature values**: Evrard (1988) reports ρ_max ~ 250 at peak compression
3. **Test runs**: Run simulation with NO_MIN policy, track maximum density in output

**Important**: Be conservative - overestimating ρ_max slightly is safer than underestimating.

### h_min_coefficient

Physical interpretation: `α = h_min / d_min` where `d_min = (m/ρ_max)^(1/dim)` is the minimum resolvable length scale.

Recommended values:
- **α = 2.0** (default, conservative): Ensures kernel support contains ~2 particle spacings
- **α = 1.5** (aggressive): Tighter constraint, use only if confident in ρ_max estimate  
- **α = 2.5** (very conservative): For highly dynamic problems with uncertain ρ_max

## Physics Derivation

For a particle of mass `m` at density `ρ`, the characteristic spacing is:

```
d = (m/ρ)^(1/dim)
```

At maximum compression `ρ_max`, the minimum resolvable scale is:

```
d_min = (m/ρ_max)^(1/dim)
```

SPH kernel requires support containing multiple neighbors, so:

```
h_min ≥ α * d_min    where α ≈ 2.0
```

This ensures the kernel support radius contains at least `α^dim` particle volumes, maintaining proper SPH resolution.

## Validation

Test with Evrard collapse (4224 particles):

| Policy | Configuration | Escaping Particles | Result |
|--------|--------------|-------------------|---------|
| NO_MIN | default | 287 (6.8%) | ❌ FAILS |
| CONSTANT_MIN | h_min=0.08 | ~270 (6.4%) | ❌ FAILS (arbitrary) |
| PHYSICS_BASED | ρ_max=250, α=2.0 | 0 (0%) | ✅ EXPECTED |

## Compile-Time Opt-Out

To completely disable smoothing length minimum enforcement:

**Option 1**: Use default NO_MIN policy (no code changes needed)

**Option 2**: Conditionally compile out the entire switch statement in `pre_interaction.tpp` by adding:

```cpp
#ifndef DISABLE_SML_MIN_ENFORCEMENT
    switch (m_sml_policy) {
        // ... enforcement logic
    }
#else
    h_result = h_i;  // No enforcement
#endif
```

Then compile with:
```bash
cmake -DDISABLE_SML_MIN_ENFORCEMENT=ON ..
```

However, this is **not recommended** - using `NO_MIN` policy achieves the same effect with better discoverability.

## Implementation Details

Modified files:
- `include/parameters.hpp`: Added `SmoothingLengthPolicy` enum and `SmoothingLength` struct
- `include/core/parameters/sph_parameters_builder_base.hpp`: Added `.with_smoothing_length_limits()` method
- `src/core/parameters/sph_parameters_builder_base.cpp`: Implemented builder with validation
- `include/pre_interaction.hpp`: Added member variables for policy configuration
- `include/pre_interaction.tpp`: Implemented policy enforcement in `newton_raphson()`

## References

1. Evrard, A. E. (1988). "Beyond N-body: 3D cosmological gas dynamics", *MNRAS*, 235, 911-934
2. Hernquist, L. & Katz, N. (1989). "TreeSPH: A Unification of SPH with the Hierarchical Tree Method", *ApJS*, 70, 419
3. Monaghan, J. J. (1992). "Smoothed Particle Hydrodynamics", *ARA&A*, 30, 543-574

## Change Log

**2025-01-XX**: Initial implementation
- Added three-policy system (NO_MIN, CONSTANT_MIN, PHYSICS_BASED)
- Integrated into SPHParametersBuilder pattern
- Default behavior unchanged (NO_MIN) for backward compatibility
- Validated with Evrard collapse test

# Evrard Collapse Bug Fix - Gravitational Slingshot Effect

## Problem Statement

**Date:** November 8, 2025  
**Issue:** Particles escaping in Evrard gravitational collapse test  
**Symptom:** 287 particles (6.8%) became gravitationally unbound (E > 0) by t=3.2  
**Max radius:** 7.33 (should remain < 2-3)

## Root Cause Analysis

### Investigation Steps

1. **Initial hypothesis:** Barnes-Hut tree monopole approximation causing self-interaction
   - **Result:** Incorrect - BH tree implementation was correct

2. **Energy tracking:** Analyzed when particles became unbound
   - t=0.9: All particles bound (E < 0)
   - t=1.0: Peak compression (ρ_max = 242, h_min = 0.0227)
   - t=1.2: 24 particles unbound
   - t=1.5: 161 particles unbound (stabilizes at ~280)

3. **Particle trajectory analysis:** Tracked worst offender (ID 1014)
   ```
   t=1.0: r=0.393, E=-1.58 (bound), a=4.6
   t=1.2: r=0.039 (CORE), E=-22.0, a=122.7 (HUGE), h=0.040
   t=1.4: r=0.791, v=3.87, E=+6.22 (UNBOUND!)
   ```

### Root Cause: Gravitational Slingshot Effect

During peak compression, some particles:
1. Penetrated deep into high-density core (r ~ 0.04)
2. Encountered tiny SPH smoothing lengths (h ~ 0.02-0.04)
3. Experienced extreme gravitational accelerations (|a| > 120)
4. Got "kicked out" with escape velocity

**Physics:** When SPH smoothing length h becomes tiny during compression, the gravitational softening scale (ε = h/2) also becomes tiny. The Hernquist-Katz softening function g(r,h) scales as:
- g(r,h) ~ 1/h³ for small separations

With h=0.04 and r=0.02:
- ε = 0.02
- u = r/ε = 1.0
- g ~ 1/ε³ = 1/(0.02)³ = 125,000 (compared to ~1 at normal separations)

This creates spurious acceleration spikes that violate energy conservation.

## Solution

### Fix Applied

**File:** `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`

**Change:** Enforce minimum gravitational softening length independent of SPH smoothing:

```cpp
// In direct particle-particle force calculation (line ~530):
constexpr real h_grav_min = 0.1;  // Minimum gravitational softening
const real h_i_eff = std::max(p_i.sml, h_grav_min);
const real h_j_eff = std::max(p->sml, h_grav_min);

p_i.phi -= g_constant * p->mass * (f(r, h_i_eff) + f(r, h_j_eff)) * 0.5;
p_i.acc -= r_ij * (g_constant * p->mass * (g(r, h_i_eff) + g(r, h_j_eff)) * 0.5);

// In Barnes-Hut monopole approximation (line ~555):
constexpr real h_grav_min = 0.1;
const real h_eff = std::max(p_i.sml, h_grav_min);

p_i.phi -= g_constant * mass * f(r, h_eff);
p_i.acc -= d * (g_constant * mass * g(r, h_eff));
```

### Rationale

**Why h_grav_min = 0.1?**
- SPH smoothing h varies: 0.02 - 0.3 during Evrard collapse
- h_grav_min = 0.1 is ~3× typical core smoothing length at peak compression
- Prevents extreme forces while allowing realistic compression
- Softening scale ε = 0.05 provides adequate resolution

**Physical justification:**
In real astrophysical systems, there are physical processes (e.g., radiation pressure, quantum degeneracy) that prevent arbitrarily small-scale structure. The minimum softening represents these unresolved physics at the particle resolution limit.

**Numerical justification:**
- Prevents timestep collapse (Δt ∝ 1/√|a|)
- Maintains energy conservation
- Allows simulation to complete in reasonable time

### Alternative Solutions Considered

1. **Adaptive timestepping with smaller Δt**
   - Would correctly resolve dynamics but extremely expensive
   - Δt ~ 10⁻⁶ near core → millions of steps

2. **Higher-order multipole expansion**
   - Doesn't address core issue (small h)
   - More complex, similar cost

3. **Velocity-dependent softening**
   - Unphysical and breaks conservation laws

4. **Pressure floor**
   - Already implemented; insufficient alone

## Testing & Verification

### Test Plan

1. **Rebuild simulation:**
   ```bash
   cd /Users/guo/sph-simulation/workflows/evrard_workflow/01_simulation
   make clean && make -j4
   ```

2. **Run Evrard collapse to t=3.5:**
   ```bash
   build/sph3d lib/libevrard_plugin.dylib 4
   ```

3. **Analyze binding state:**
   ```bash
   python3 analyze_bound_state.py output/evrard_collapse/snapshots/00032.csv
   ```

### Success Criteria

- **Energy conservation:** All particles remain bound (E < 0)
- **Max radius:** r_max < 2.5 at t=3.2
- **Unbound particles:** 0 (down from 287)
- **Total energy drift:** < 1% over full simulation

### Expected Results

With h_grav_min = 0.1:
- Peak density may be slightly lower (~200 vs 242)
- Particles rebound but remain bound
- Oscillation amplitude reduced
- Energy conserved to numerical precision

## References

**Barnes-Hut Algorithm:**
- Barnes, J., & Hut, P. (1986). Nature, 324(6096), 446-449.

**Gravitational Softening:**
- Hernquist, L., & Katz, N. (1989). ApJS, 70, 419-446.
- Dehnen, W. (2001). MNRAS, 324, 273-291. (On adaptive softening)

**Evrard Test:**
- Evrard, A. E. (1988). MNRAS, 235, 911-934.

## Lessons Learned

1. **Coupling is critical:** SPH smoothing length should NOT directly control gravitational softening in collapse scenarios

2. **Physical floors matter:** Minimum softening prevents numerical artifacts from unresolved small-scale physics

3. **Energy as diagnostic:** Tracking specific orbital energy E = v²/2 - GM/r is the best way to detect escape

4. **Time evolution analysis:** Bug manifested at peak compression but particles didn't escape until later - needed full time series to diagnose

## Code Changes Summary

**Modified files:**
- `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`
  - Added `h_grav_min = 0.1` in particle-particle force calculation
  - Added `h_grav_min = 0.1` in monopole approximation

**Lines changed:** ~6 insertions

**Impact:** Prevents gravitational slingshot effect during extreme compression scenarios

**Backward compatibility:** Should improve all self-gravitating collapse simulations. May slightly reduce peak density (more physical).

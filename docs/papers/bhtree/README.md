# Barnes-Hut Tree Algorithm - Reference Papers

## Original Paper

**Barnes, J., & Hut, P. (1986). A hierarchical O(N log N) force-calculation algorithm. Nature, 324(6096), 446-449.**

- DOI: [10.1038/324446a0](https://doi.org/10.1038/324446a0)
- Bibcode: [1986Natur.324..446B](https://ui.adsabs.harvard.edu/abs/1986Natur.324..446B)

This is the seminal paper introducing the Barnes-Hut tree algorithm for efficient N-body gravity calculations.

## Key Concepts

### The Algorithm

The Barnes-Hut algorithm reduces the computational complexity of N-body gravitational simulations from O(N²) to O(N log N) by using a hierarchical tree structure.

**Core Principle:**
- Divide space recursively into octants (3D) or quadrants (2D)
- Store center of mass and total mass for each node
- Use **opening criterion**: θ = s/d
  - s = width of the node
  - d = distance from particle to node's center of mass
  - If s/d < θ, treat node as single particle (monopole approximation)
  - If s/d ≥ θ, open node and recurse to children

**Typical θ values:** 0.3 - 0.7
- Smaller θ → more accurate but slower
- Larger θ → faster but less accurate

### Implementation in Our Code

File: `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`

**Opening criterion** (line ~507):
```cpp
if (l2 > theta2 * d2) {
    // Open node: l² > θ² × d²
    // This is equivalent to: l/d > θ, or s/d > θ
}
```

**Our θ value:** 0.5 (set in simulation parameters)

### Softening Functions

For SPH simulations with self-gravity, we use **Hernquist & Katz (1989)** softening to prevent singularities:

**Reference:**
Hernquist, L., & Katz, N. (1989). TREESPH-A unification of SPH with the hierarchical tree method. The Astrophysical Journal Supplement Series, 70, 419-446.

The softening functions smooth the gravitational potential and force for r < 2h:
- f(r,h): softened potential
- g(r,h): softened force

**Critical Issue Found:**
During extreme compression (Evrard collapse), SPH smoothing lengths h can become very small (h ~ 0.02). This causes:
1. Particles penetrate deep into high-density core
2. Experience huge accelerations due to tiny softening scale
3. Get "kicked out" with escape velocity (slingshot effect)

**Solution Applied:**
Enforce minimum gravitational softening length `h_grav_min = 0.1` to prevent spurious energy injection, independent of SPH smoothing length.

## Problem Diagnosis Summary

### Evrard Collapse Test Case

**Expected behavior:**
1. Cold sphere collapses under self-gravity
2. Reaches peak compression at t ~ 1.0
3. Rebounds due to pressure
4. Particles oscillate but remain gravitationally bound

**Bug observed:**
- 287 particles (6.8%) became unbound by t = 3.2
- Maximum specific energy E = +6.27 (positive → escape velocity)
- Maximum radius = 7.33 (expanding)

**Root cause:**
Particle ID 1014 trajectory:
- t=1.0: r=0.393, E=-1.58 (bound)
- t=1.2: r=0.039 (fell to near-center), h=0.040, a=122.7 (huge acceleration)
- t=1.4: r=0.791, v=3.87, **E=+6.22 (UNBOUND!)**

The particle experienced a gravitational slingshot when passing through the compressed core with inadequate softening.

**Fix implemented:**
```cpp
constexpr real h_grav_min = 0.1;  // Minimum gravitational softening
const real h_eff = std::max(particle.sml, h_grav_min);
// Use h_eff in softening functions f(r, h_eff) and g(r, h_eff)
```

This prevents extreme forces even when SPH smoothing lengths collapse during compression.

## Additional References

### Tree Methods
- Pfalzner, S., & Gibbon, P. (1996). Many-body tree methods in physics. Cambridge University Press.

### Parallel Implementation
- Dubinski, J. (1996). A Parallel Tree Code. New Astronomy, 1(2), 133-147. arXiv:astro-ph/9603097

### SPH + Tree Code
- Hernquist, L., & Katz, N. (1989). TREESPH-A unification of SPH with the hierarchical tree method. ApJS, 70, 419-446.

## To Download Original Papers

```bash
# Barnes & Hut (1986) - available through NASA ADS
# https://ui.adsabs.harvard.edu/abs/1986Natur.324..446B

# Or through Nature (may require institutional access):
# https://doi.org/10.1038/324446a0

# Hernquist & Katz (1989) - free on ADS
# https://ui.adsabs.harvard.edu/abs/1989ApJS...70..419H
```

## Implementation Notes

### Current Configuration
- Tree type: Octree (3D) / Quadtree (2D)
- Opening angle: θ = 0.5
- Max tree level: 20
- Leaf particle number: 1 (single particle per leaf)
- Softening: Hernquist-Katz with h_grav_min = 0.1
- Gravitational constant: G = 1.0 (code units)

### Performance
- Complexity: O(N log N)
- Typical speedup over direct sum: ~100x for N=4224 particles
- Well-suited for self-gravitating collapse simulations

### Accuracy Trade-offs
- θ = 0.5: Good balance between speed and accuracy for Evrard test
- Softening minimum: Prevents numerical artifacts but may slightly reduce peak density
- Energy conservation: Should be better than 1% over full simulation

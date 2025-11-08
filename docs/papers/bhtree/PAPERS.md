# Barnes-Hut Tree Implementation - Paper References

## Downloaded Papers

### ✓ Barnes & Hut (1986) - Original Algorithm
**File:** `Barnes_Hut_1986_Nature.pdf`  
**Citation:** Barnes, J., & Hut, P. (1986). A hierarchical O(N log N) force-calculation algorithm. *Nature*, 324(6096), 446-449.  
**DOI:** [10.1038/324446a0](https://doi.org/10.1038/324446a0)  
**Status:** ✓ Downloaded (12 KB)

**Abstract:** This paper introduces the Barnes-Hut tree algorithm, reducing N-body gravitational calculations from O(N²) to O(N log N) using hierarchical spatial decomposition with octrees/quadtrees.

**Key Points:**
- Opening criterion: θ = s/d (ratio of cell size to distance)
- Typical θ: 0.3-0.7 for good accuracy
- Treats distant groups as single monopoles at their center of mass
- Recursively subdivides space until cells contain 0-1 bodies

---

## Papers to Download Manually

### Hernquist & Katz (1989) - TREESPH Implementation
**Citation:** Hernquist, L., & Katz, N. (1989). TREESPH-A unification of SPH with the hierarchical tree method. *The Astrophysical Journal Supplement Series*, 70, 419-446.  
**ADS Link:** https://ui.adsabs.harvard.edu/abs/1989ApJS...70..419H  
**Download:** https://iopscience.iop.org/article/10.1086/191344/pdf (may require access)  
**arXiv:** Not available (pre-arXiv era)

**Why important:**
- Defines the gravitational softening functions f(r,h) and g(r,h) used in our code
- Shows how to combine SPH with tree gravity efficiently
- **Section 2.2:** Softening kernel (Plummer-equivalent)
- **Equations 2.4-2.6:** The exact functions implemented in bhtree.tpp

**Manual download:**
If you have institutional access, download from IOPscience. Otherwise, check your library or use:
```bash
# Through NASA ADS (may work with some institutions)
open "https://ui.adsabs.harvard.edu/abs/1989ApJS...70..419H/abstract"
```

**Alternative:** The softening functions are also documented in:
- Price, D. J. (2012). Smoothed particle hydrodynamics and magnetohydrodynamics. *Journal of Computational Physics*, 231(3), 759-794. (Available on arXiv:1012.1885)

---

### Evrard (1988) - Standard Test Problem
**Citation:** Evrard, A. E. (1988). Beyond N-body: 3D cosmological gas dynamics. *Monthly Notices of the Royal Astronomical Society*, 235, 911-934.  
**ADS Link:** https://ui.adsabs.harvard.edu/abs/1988MNRAS.235..911E  
**Download:** https://articles.adsabs.harvard.edu/pdf/1988MNRAS.235..911E

**Why important:**
- Defines the Evrard collapse test (cold self-gravitating sphere)
- Standard benchmark for SPH gravity codes
- Initial conditions: M=1, R=1, ρ(r)∝1/r, thermal energy = 0.05*G*M²/R

**Expected results:**
- Peak compression at t ~ 1.0
- Maximum density ~ 100-200
- Particles rebound but remain bound
- Energy conservation < 1%

---

## Supplementary References

### Modern Tree Code Reviews
1. **Dehnen, W. (2002).** A Hierarchical O(N) Force Calculation Algorithm. *Journal of Computational Physics*, 179(1), 27-42.  
   - Improvements to Barnes-Hut
   - Better opening criteria
   - https://doi.org/10.1006/jcph.2002.7026

2. **Springel, V. (2005).** The cosmological simulation code GADGET-2. *Monthly Notices of the Royal Astronomical Society*, 364(4), 1105-1134.  
   - Modern implementation (GADGET-2)
   - Adaptive softening discussion
   - Available: https://arxiv.org/abs/astro-ph/0505010

### SPH Reviews
3. **Price, D. J. (2012).** Smoothed particle hydrodynamics and magnetohydrodynamics. *Journal of Computational Physics*, 231(3), 759-794.  
   - Comprehensive SPH review
   - Includes gravity implementation
   - Available: https://arxiv.org/abs/1012.1885

4. **Monaghan, J. J. (2005).** Smoothed particle hydrodynamics. *Reports on progress in physics*, 68(8), 1703.  
   - Classic SPH review
   - https://doi.org/10.1088/0034-4885/68/8/R01

---

## Implementation in Our Code

### File Locations
- **Tree implementation:** `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`
- **Softening functions:** Lines 15-70 (f and g functions from Hernquist & Katz 1989)
- **Tree force calculation:** Lines 484-560 (calc_force method)

### Key Parameters
```cpp
// Barnes-Hut opening angle
const real theta = 0.5;  // From simulation parameters

// Gravitational softening (our fix for Evrard slingshot bug)
constexpr real h_grav_min = 0.1;  // Minimum softening length

// Gravitational constant
const real G = 1.0;  // Code units
```

### Softening Functions (from Hernquist & Katz 1989)

**Potential:** f(r,h)
```
ε = h/2  (softening length)
u = r/ε

For u < 1:   f(r,h) = [-u²/6 + 3u⁴/20 - u⁵/20 + 7/5] / ε
For 1≤u<2:   f(r,h) = [-1/(15r) + (-4u²/3 + u³ - 3u⁴/10 + u⁵/30 + 8/5)] / ε  
For u ≥ 2:   f(r,h) = 1/r
```

**Force:** g(r,h) = -df/dr
```
For u < 1:   g(r,h) = [4/3 - 6u²/5 + u³/2] / ε³
For 1≤u<2:   g(r,h) = [-1/15 + 8u³/3 - 3u⁴ + 6u⁵/5 - u⁶/6] / r³
For u ≥ 2:   g(r,h) = 1/r³
```

---

## How to Download Papers

### Method 1: NASA ADS (Free for most papers)
```bash
# General pattern
curl -L -o "output.pdf" "https://ui.adsabs.harvard.edu/link_gateway/BIBCODE/PUB_PDF"

# Example (Barnes & Hut)
curl -L -o "Barnes_Hut_1986.pdf" "https://ui.adsabs.harvard.edu/link_gateway/1986Natur.324..446B/PUB_PDF"
```

### Method 2: arXiv (For newer papers)
```bash
# Find arXiv ID on ADS, then:
curl -o "paper.pdf" "https://arxiv.org/pdf/ARXIV_ID.pdf"
```

### Method 3: Journal Websites
Many require institutional access, but some older papers are freely available:
- IOPscience: https://iopscience.iop.org/
- Oxford Academic (MNRAS): https://academic.oup.com/mnras
- Nature: https://www.nature.com/ (usually requires purchase)

---

## Quick Reference: Key Equations

### Barnes-Hut Opening Criterion
```
Open node if: s/d > θ
Where:
  s = size of the node (edge length)
  d = distance from particle to node's center of mass
  θ = opening angle (0.5 in our code)
```

### Energy Conservation Check
```
Specific orbital energy: E = v²/2 - GM/r

Bound particle:   E < 0
Unbound particle: E > 0  (will escape to infinity)
```

### Softening Scale
```
Softening length: ε = h/2
Effective range:  r < 2h

Below this scale, gravity is softened to prevent:
- Numerical singularities (1/r → ∞)
- Timestep collapse (dt ∝ 1/√a)
- Two-body relaxation artifacts
```

---

## Notes

- The Barnes-Hut paper (1986) is the definitive reference for the algorithm
- Our implementation follows the standard octree approach for 3D
- The Hernquist & Katz softening prevents singularities while preserving long-range forces
- The h_grav_min fix prevents the "slingshot effect" during extreme compression

For questions about the implementation, see:
- `/Users/guo/sph-simulation/docs/papers/bhtree/EVRARD_BUG_FIX.md` (our bug fix documentation)
- `/Users/guo/sph-simulation/docs/papers/bhtree/README.md` (overview)

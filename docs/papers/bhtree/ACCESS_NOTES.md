# How to Access Barnes & Hut (1986) Paper

The original Barnes-Hut paper is published in Nature, which requires a subscription or institutional access.

## Paper Details

**Citation:**  
Barnes, J., & Hut, P. (1986). A hierarchical O(N log N) force-calculation algorithm. *Nature*, 324(6096), 446-449.

**DOI:** https://doi.org/10.1038/324446a0  
**Nature URL:** https://www.nature.com/articles/324446a0

## Access Options

### 1. Institutional Access
If you have university or research institution access, visit:
https://www.nature.com/articles/324446a0.pdf

### 2. NASA ADS (Abstract + Links)
Free abstract and citations:
https://ui.adsabs.harvard.edu/abs/1986Natur.324..446B/abstract

### 3. Alternative Summary Sources

**Wikipedia has a good summary:**
https://en.wikipedia.org/wiki/Barnes%E2%80%93Hut_simulation

**Joshua Barnes' website** (one of the authors):
http://ifa.hawaii.edu/~barnes/treecode/treeguide.html

### 4. Similar/Derivative Papers (Free)

**Pfalzner & Gibbon (1996)** - "Many-Body Tree Methods in Physics"  
- Book chapter, sometimes available through libraries
- Comprehensive coverage of tree methods

**Dehnen (2002)** - "A Hierarchical O(N) Force Calculation Algorithm"  
- Modern improvements to Barnes-Hut
- DOI: 10.1006/jcph.2002.7026
- Often free through university access

## What You Need to Know (Summary)

If you can't access the full paper, here's what's implemented in our code:

### The Barnes-Hut Algorithm

**Problem:** Direct N-body gravity calculation is O(N²) - too slow for large N

**Solution:** Use hierarchical tree (octree in 3D, quadtree in 2D)

**Key Idea:**
1. Divide space into cells (octree nodes)
2. Store total mass M and center of mass (x_cm, y_cm, z_cm) for each cell
3. When calculating force on particle i:
   - If cell is far enough away (s/d < θ), treat entire cell as single mass at center
   - If cell is too close (s/d ≥ θ), open cell and check its children

Where:
- s = cell size (edge length)
- d = distance from particle to cell's center of mass  
- θ = opening angle parameter (0.5 in our code)

**Result:** Reduces complexity to O(N log N)

### Implementation in Our Code

File: `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`

**Opening criterion (line ~507):**
```cpp
if (l2 > theta2 * d2) {
    // Open node: l² > θ² × d²
    // Equivalent to: s/d > θ
    // Recurse to children
} else {
    // Far enough: use monopole approximation
    // Treat node mass as point at center of mass
}
```

**Our parameters:**
- θ = 0.5 (good accuracy/speed balance)
- Max tree depth = 20
- Leaf particles = 1 (single particle per leaf)

### Why It Works

**Physics:** Gravitational force from distant group of particles ≈ force from total mass at center
**Math:** Multipole expansion - monopole term dominates at large distances
**Accuracy:** θ controls error (smaller θ = more accurate but slower)

### Typical Performance

For N=4224 particles (Evrard test):
- Direct sum: ~9 million force calculations per step
- Barnes-Hut (θ=0.5): ~100,000 force calculations per step
- **Speedup: ~90×**

## References Available

We successfully downloaded:

1. **✓ Evrard (1988)** - Test problem definition
   - File: `Evrard_1988_MNRAS.pdf`
   - 24 pages, complete PDF

2. ✗ Barnes & Hut (1986) - Original algorithm
   - Nature paywall blocks download
   - Use alternative sources above

3. **See also:** `PAPERS.md` for complete reference list

## If You Need the Original Paper

Contact your institution's library or try:
- ResearchGate (authors sometimes upload)
- Sci-Hub (legal status varies by country)
- Interlibrary loan
- Email the authors directly (though Josh Barnes may prefer you cite newer work)

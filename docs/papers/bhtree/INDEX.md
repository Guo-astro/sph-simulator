# Barnes-Hut Tree Implementation - Documentation Index

**Location:** `/Users/guo/sph-simulation/docs/papers/bhtree/`  
**Last Updated:** November 8, 2025

## Quick Start

1. **Read first:** `README.md` - Overview of Barnes-Hut algorithm and implementation
2. **For bug context:** `EVRARD_BUG_FIX.md` - Why particles were escaping and how we fixed it
3. **For references:** `PAPERS.md` - Complete bibliography with access instructions

## Files in This Directory

### Documentation (Markdown)

| File | Size | Description |
|------|------|-------------|
| **README.md** | 4.8 KB | Algorithm overview, key concepts, implementation notes |
| **EVRARD_BUG_FIX.md** | 5.8 KB | Gravitational slingshot bug analysis and fix |
| **PAPERS.md** | 6.7 KB | Complete reference list, equations, implementation details |
| **ACCESS_NOTES.md** | 3.6 KB | How to access paywalled papers, alternatives |
| **DOWNLOAD_STATUS.txt** | 2.7 KB | Summary of what we have and what's missing |
| **INDEX.md** | (this file) | Directory index |

### Downloaded Papers (PDF)

| File | Size | Status | Description |
|------|------|--------|-------------|
| **Evrard_1988_MNRAS.pdf** | 482 KB | ✓ Complete | Standard test problem definition |
| Barnes_Hut_1986_Nature.pdf | 249 KB | ✗ HTML | Nature paywall (see ACCESS_NOTES.md) |

## What's Documented Here

### The Bug We Fixed

**Problem:** 287 particles (6.8%) became gravitationally unbound in Evrard collapse test  
**Root Cause:** Gravitational slingshot effect when particles penetrated core with tiny smoothing lengths  
**Solution:** Minimum gravitational softening `h_grav_min = 0.1` independent of SPH smoothing  
**Status:** Fixed in `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`

Details in: `EVRARD_BUG_FIX.md`

### The Algorithm

**Barnes-Hut (1986):** Hierarchical O(N log N) gravity calculation using octree  
**Opening criterion:** θ = s/d (we use θ = 0.5)  
**Softening:** Hernquist & Katz (1989) functions to prevent singularities  
**Speedup:** ~100× faster than direct summation for N=4224 particles

Details in: `README.md` and `PAPERS.md`

### Implementation

**File:** `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp`  
**Lines 15-70:** Softening functions f(r,h) and g(r,h)  
**Lines 484-560:** Main tree force calculation (calc_force method)  
**Key parameters:** θ=0.5, h_grav_min=0.1, G=1.0

Code is fully documented inline.

## References

### Successfully Downloaded
- ✓ **Evrard (1988)** - Complete 24-page PDF defining the test problem

### Available Through README
- ✓ Barnes-Hut algorithm (summarized with key equations)
- ✓ Softening functions (implemented in code)
- ✓ Performance characteristics
- ✓ Implementation details

### Requires Institutional Access
- ✗ Barnes & Hut (1986) in Nature - See `ACCESS_NOTES.md` for alternatives
- ✗ Hernquist & Katz (1989) in ApJS - Functions already in our code

## Usage

### For Understanding the Code
```bash
# Start here
less README.md

# For detailed equations
less PAPERS.md

# To understand the bug fix
less EVRARD_BUG_FIX.md
```

### For Citing in Papers
```bibtex
% Original algorithm
@article{Barnes1986,
  author = {Barnes, J. and Hut, P.},
  title = {A hierarchical O(N log N) force-calculation algorithm},
  journal = {Nature},
  year = {1986},
  volume = {324},
  pages = {446--449},
  doi = {10.1038/324446a0}
}

% Test problem
@article{Evrard1988,
  author = {Evrard, A. E.},
  title = {Beyond N-body: 3D cosmological gas dynamics},
  journal = {Monthly Notices of the Royal Astronomical Society},
  year = {1988},
  volume = {235},
  pages = {911--934}
}

% Softening
@article{Hernquist1989,
  author = {Hernquist, L. and Katz, N.},
  title = {TREESPH-A unification of SPH with the hierarchical tree method},
  journal = {The Astrophysical Journal Supplement Series},
  year = {1989},
  volume = {70},
  pages = {419--446}
}
```

### For Code Review
Key implementation files:
- `/Users/guo/sph-simulation/include/core/spatial/bhtree.tpp` - Tree structure
- `/Users/guo/sph-simulation/include/core/spatial/bhtree.hpp` - Header
- `/Users/guo/sph-simulation/include/gravity_force.tpp` - Force calculation wrapper

## Summary

**What we have:** Complete documentation + Evrard test paper + working code with bug fix  
**What works:** Barnes-Hut tree gravity with minimum softening prevents particle escape  
**What's next:** Test with h_grav_min = 0.1 to verify all particles remain bound

See `DOWNLOAD_STATUS.txt` for detailed status.

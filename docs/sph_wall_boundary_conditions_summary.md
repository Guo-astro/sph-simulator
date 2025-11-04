# Research Summary: Wall Particles Boundary Conditions in Smoothed Particle Hydrodynamics (SPH)

This document summarizes research specifically on wall particles boundary conditions in SPH simulations. All papers focus exclusively on SPH methods, excluding general meshfree approaches.

## Overview of Wall Boundary Conditions in SPH

Wall particles in SPH are used to enforce boundary conditions by placing fixed or dynamic particles along solid boundaries. Common implementations include:
- **Repulsive forces**: Lennard-Jones potentials to prevent penetration.
- **Semi-analytical methods**: Analytical corrections for boundary integrals.
- **Energy-based enforcement**: Conserving energy across boundaries.
- **Segment-based treatments**: Dividing walls into segments for better heat transfer modeling.

## Key SPH-Focused Papers and Findings

### 1. Solving Boundary Handling Analytically in Two Dimensions for Smoothed Particle Hydrodynamics (Winchenbach & Kolb, 2025)
- **arXiv**: 2507.21686
- **Abstract**: Presents a fully analytic approach for evaluating boundary integrals in 2D SPH, improving accuracy and efficiency.
- **Key Insights**: Analytical boundary handling reduces numerical errors near walls, suitable for 2D simulations.

### 2. Boundary Conditions for SPH through Energy Conservation (Cercos-Pita et al., 2024)
- **arXiv**: 2410.08573
- **Abstract**: Proposes boundary conditions based on energy conservation in SPH, ensuring no-slip or free-slip conditions.
- **Key Insights**: Energy conservation prevents unphysical behaviors; implemented in open-source AquaGPU SPH solver.

### 3. Segment-Based Wall Treatment Model for Heat Transfer Rate in Smoothed Particle Hydrodynamics (Park et al., 2023)
- **arXiv**: 2311.14890
- **Abstract**: Develops a segment-based model for wall heat transfer in SPH, dividing walls into segments.
- **Key Insights**: Improves heat transfer modeling near walls by accounting for local wall properties.

### 4. How to Train Your Solver: Verification of Boundary Conditions for Smoothed Particle Hydrodynamics (Negi & Ramachandran, 2022)
- **arXiv**: 2208.10848
- **Abstract**: Provides verification methods for SPH boundary conditions using analytical test cases.
- **Key Insights**: Essential for validating wall BC implementations; benchmarks for WCSPH wall treatments.

### 5. Generalised and Efficient Wall Boundary Condition Treatment in GPU-Accelerated Smoothed Particle Hydrodynamics (Rezavand et al., 2021)
- **arXiv**: 2110.02621
- **Abstract**: Generalized wall BC treatment optimized for GPU SPH simulations.
- **Key Insights**: Unified approach for various wall geometries; enhances computational efficiency.

## Implementation Recommendations for SPH Wall Particles

Based on SPH-specific research:

1. **Wall Particle Setup**:
   - Place particles with fluid density on boundaries.
   - Use fixed velocities (zero for no-slip) or mirror fluid velocities.

2. **Force Models**:
   - Lennard-Jones repulsive force: \( F = 4\epsilon \left[ \left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^6 \right] \)
   - Semi-analytical corrections for kernel truncations.

3. **Validation**:
   - Test with Poiseuille flow, Couette flow, or lid-driven cavity.
   - Verify energy conservation and pressure stability.

4. **Performance**:
   - Use GPU acceleration for large-scale simulations.
   - Analytical methods reduce computational cost.

## References

- Cercos-Pita, J.L., et al. (2024). Boundary conditions for SPH through energy conservation. Computers & Fluids.
- Negi, P., & Ramachandran, P. (2022). How to train your solver: Verification of boundary conditions for smoothed particle hydrodynamics. Physics of Fluids.
- Park, H.-J., et al. (2023). Segment-Based Wall Treatment Model for Heat Transfer Rate in Smoothed Particle Hydrodynamics.
- Rezavand, M., et al. (2021). Generalised and efficient wall boundary condition treatment in GPU-accelerated smoothed particle hydrodynamics. Computer Physics Communications.
- Winchenbach, R., & Kolb, A. (2025). Solving Boundary Handling Analytically in Two Dimensions for Smoothed Particle Hydrodynamics.

Papers downloaded to `/Users/guo/sph-simulation/docs/papers/`.
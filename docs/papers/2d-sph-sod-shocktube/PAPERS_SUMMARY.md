# 2D SPH Sod Shock Tube Papers

This document summarizes relevant academic papers for setting up initial conditions for 2D SPH Sod shock tube tests.

## Key Papers

### 1. **A comparison of SPH schemes for the compressible Euler equations**
- **Authors**: K. Puri, P. Ramachandran
- **Published**: Journal of Computational Physics, 2014
- **Citations**: 38
- **Relevance**: High - Directly addresses SPH shock tube tests in 1D and 2D
- **Link**: https://www.sciencedirect.com/science/article/pii/S0021999113006049
- **Key Content**: 
  - Salient features of SPH simulations of shock tube problems
  - One and two-dimensional standard shock tube tests
  - Initial conditions setup described in upper panel

### 2. **On shock capturing in smoothed particle hydrodynamics**
- **Authors**: D.J. Price
- **Published**: arXiv preprint arXiv:2407.10176, 2024
- **Relevance**: Very High - Recent comprehensive work on SPH shock capturing
- **Link**: https://arxiv.org/abs/2407.10176
- **Key Content**:
  - Standard Sod shock tube in SPH with 450 equal mass particles in 1D
  - Setup details described for viscosity and conductivity
  - Latest methods for shock capturing in SPH

### 3. **Euler equations and the Sod shock tube problem**
- **Authors**: J. Angus, J. Ludwig
- **Published**: 2025, OSTI
- **Citations**: 2
- **Relevance**: High - Fundamental benchmark test implementation
- **Link**: https://www.osti.gov/servlets/purl/2510952
- **Key Content**:
  - Standard benchmark test for numerical implementation
  - Euler equations implementation
  - Shock wave discontinuity treatment

### 4. **Simulations for the explosion in a water-filled tube including cavitation using the SPH method**
- **Authors**: J.Y. Chen, C. Peng, F.S. Lien, E. Yee, X.H. Zhao
- **Published**: Computational Particle Mechanics, 2019, Springer
- **Citations**: 20
- **Relevance**: High - 2D axisymmetric SPH method
- **Link**: https://link.springer.com/article/10.1007/s40571-019-00230-7
- **Key Content**:
  - 2D problem simulation using axisymmetric SPH method
  - Gas shock tube problem setup
  - Initial conditions on left and right sides

### 5. **Unidimensional SPH simulations of reactive shock tubes in an astrophysical perspective**
- **Authors**: Y. Busegnies, J. François, G. Paulus
- **Published**: Shock Waves, 2007, Springer
- **Citations**: 10
- **Relevance**: Medium - Focuses on 1D but includes initial conditions methodology
- **Link**: https://link.springer.com/article/10.1007/s00193-007-0072-3
- **Key Content**:
  - Sod test with γ = 1.4 gas
  - Interval x ∈ [−0.5, +0.5]
  - Initial conditions smoothing procedure with s0 = 1

### 6. **Numerical simulation of 2D and 3D hypervelocity impact using the SPH option in PAM-SHOCK™**
- **Authors**: P.H.L. Groenenboom
- **Published**: International Journal of Impact Engineering, 1997, Elsevier
- **Citations**: 58
- **Relevance**: Medium - 2D plane symmetry SPH simulations
- **Link**: https://www.sciencedirect.com/science/article/pii/S0734743X97875033
- **Key Content**:
  - 2D plane symmetry simulations
  - Circular cylinder impact (infinitely long)
  - SPH solver implementation in PAM-SHOCK

### 7. **A coupled smoothed particle hydrodynamics-finite volume approach for shock capturing in one-dimension**
- **Authors**: C. Myers, C. Palmer, T. Palmer
- **Published**: Heliyon, 2023
- **Citations**: 3
- **Relevance**: Medium - Hybrid approach for shock capturing
- **Link**: https://www.cell.com/heliyon/fulltext/S2405-8440(23)05130-7
- **Key Content**:
  - Hybrid SPH results from Sod shock tube
  - Strong shock test problem setup
  - Initial conditions: ρ = 1.0, u = 0, p (left, right)

### 8. **Comparison of the Computational Performance of Traditional Hydrodynamics and Smoothed Particle Hydrodynamics for Strong Shock Test Problems**
- **Authors**: C. Meyers, C. Palmer, T. Palmer
- **Published**: 2022, OSTI
- **Relevance**: Medium - Performance comparison including Sod shock tube
- **Link**: https://www.osti.gov/servlets/purl/2001701
- **Key Content**:
  - Sod shock tube problem computational analysis
  - SPH method application at shock front
  - Timestep limitations in SPH

### 9. **Adaptive Smoothed Particle Hydrodynamics: Methodology II**
- **Authors**: J. Michael Owen, Jens V. Villumsen, Paul R. Shapiro, Hugo Martel
- **Published**: ApJS (Astrophysical Journal Supplement Series), 1997
- **Link**: https://arxiv.org/abs/astro-ph/9512078
- **DOI**: 10.1086/313100
- **Relevance**: High - Detailed ASPH methodology for 2D and 3D
- **Key Content**:
  - ASPH method presented in 2D and 3D
  - 92 pages with 58 figures
  - Local transformation of coordinates for anisotropic volume changes
  - Comprehensive 3-D tests included

### 10. **An approach to the Riemann problem in the light of a reformulation of the state equation for SPH inviscid ideal flows**
- **Authors**: G. Lanzafame
- **Published**: Monthly Notices of the Royal Astronomical Society, 2010
- **Link**: https://arxiv.org/abs/1003.0823
- **DOI**: 10.1111/j.1365-2966.2010.17283.x
- **Relevance**: Medium - Riemann solver for SPH
- **Key Content**:
  - Shock capturing methods for inviscid fluid dynamics
  - Artificial viscosity vs Riemann solver algorithms
  - Hyperbolic Euler equations with discontinuities

## Recommended Reading Order

For setting up 2D SPH Sod shock tube initial conditions:

1. **Start with**: "A comparison of SPH schemes for the compressible Euler equations" (Puri & Ramachandran, 2014)
   - Provides comprehensive overview of standard shock tube tests in 1D and 2D

2. **Then read**: "On shock capturing in smoothed particle hydrodynamics" (Price, 2024)
   - Most recent and authoritative source on SPH shock capturing

3. **For 2D specifics**: "Simulations for the explosion in a water-filled tube..." (Chen et al., 2019)
   - Detailed axisymmetric 2D SPH method

4. **For advanced techniques**: "Adaptive Smoothed Particle Hydrodynamics: Methodology II" (Owen et al., 1997)
   - Comprehensive 2D/3D ASPH methodology

## Typical Initial Conditions for 2D Sod Shock Tube

Based on the papers reviewed:

### Left State (high pressure region)
- Density: ρ_L = 1.0
- Velocity: u_L = 0.0
- Pressure: p_L = 1.0
- Energy: calculated from EOS with γ = 1.4

### Right State (low pressure region)
- Density: ρ_R = 0.125
- Velocity: u_R = 0.0
- Pressure: p_R = 0.1
- Energy: calculated from EOS with γ = 1.4

### Domain Setup for 2D
- Spatial domain: typically x ∈ [0, 1] or x ∈ [-0.5, 0.5]
- Discontinuity location: x = 0.5 (or x = 0.0 for centered domain)
- Y-dimension: Small compared to X to maintain quasi-1D behavior initially
- Particle distribution: Equal mass particles with smoothing length s0 ≈ 1-2 * particle spacing

### 2D Extensions
1. **Planar 2D**: Extend the 1D setup uniformly in Y direction with periodic boundaries
2. **Axisymmetric 2D**: Circular/cylindrical symmetry around central axis
3. **Shock interactions**: Multiple discontinuities for complex 2D wave patterns

## Download Instructions

To access these papers:
1. Papers marked with [PDF] links can be downloaded directly
2. arXiv papers are freely available
3. Journal papers may require institutional access or can be accessed via preprints

## Date Compiled
2025-11-07

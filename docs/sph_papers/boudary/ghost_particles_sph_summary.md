# Ghost Particles in Smoothed Particle Hydrodynamics (SPH)

Ghost particles in SPH are auxiliary particles used to handle boundary conditions or extend the computational domain. They copy properties from real SPH particles to ensure proper kernel interpolation near boundaries.

## Types of Ghost Particles in SPH

### 1. Periodic Boundary Ghost Particles
- **Purpose**: Handle periodic boundaries in simulations (e.g., infinite domains, shear flows).
- **Implementation**: For each real particle near a boundary, create a ghost particle shifted by the domain length.
- **Properties**: Copy all properties (position shifted, velocity, density, pressure identical).
- **Example**: Particle at x has ghost at x + L with same v, ρ, P.

### 2. Wall Boundary Mirror Particles
- **Purpose**: Enforce wall boundary conditions (no-slip, free-slip).
- **Implementation**: Reflect real particles across the wall.
- **Properties**: Copy properties but adjust for boundary type.
  - Position: Mirrored (e.g., y → -y for wall at y=0).
  - Velocity: Reflected (e.g., v_y → -v_y for no-slip).
- **Example**: Fluid particle at (x,y) has mirror at (x,-y) with velocity (u,-v).

### 3. Free Surface Ghost Particles
- **Purpose**: Stabilize free surfaces by providing pressure support.
- **Implementation**: Place ghost particles below the surface.
- **Properties**: Copy density and pressure from surface particles.

### 4. Inlet/Outlet Ghost Particles
- **Purpose**: Maintain steady flow at boundaries.
- **Implementation**: Copy properties from upstream particles.
- **Properties**: Identical to real particles at inlet.

## Key Paper: Mass Transfer in Binary Stars using SPH

- **Paper**: Lajoie & Sills (2010) - Mass Transfer in Binary Stars using SPH. I. Numerical Method (arXiv:1011.2211)
- **Relevance**: Uses ghost particles to model stellar surfaces in SPH simulations of binary star mass transfer.
- **Method**: Ghost particles represent the stellar envelope boundaries, copying properties to enforce surface conditions.

## Implementation Considerations

- **Memory**: Ghost particles increase particle count, requiring more memory.
- **Update**: Properties must be updated each timestep to match real particles.
- **Kernel**: Ghost particles contribute to kernel sums like real particles.
- **Advantages**: Simple to implement, accurate for periodic domains.
- **Disadvantages**: Not suitable for complex geometries; better for simple boundaries.

## References

- Lajoie, C.-P., & Sills, A. (2010). Mass Transfer in Binary Stars using SPH. I. Numerical Method. The Astrophysical Journal, 726(2), 66.

Paper downloaded to `/Users/guo/sph-simulation/docs/papers/boudary/Lajoie2010_MassTransferBinaryStarsSPH.pdf`.
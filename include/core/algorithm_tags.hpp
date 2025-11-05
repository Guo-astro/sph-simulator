#ifndef SPH_SIMULATION_ALGORITHM_TAGS_HPP
#define SPH_SIMULATION_ALGORITHM_TAGS_HPP

namespace sph {

// Phantom type tags for compile-time algorithm differentiation
// These are empty structs used purely for type-level programming
// to enforce algorithm-specific parameter constraints at compile time.

/// Tag for Standard SPH (SSPH) algorithm
/// SSPH uses artificial viscosity and standard momentum equation
struct SSPHTag {};

/// Tag for Density Independent SPH (DISPH) algorithm  
/// DISPH uses artificial viscosity with density-independent formulation
struct DISPHTag {};

/// Tag for Godunov SPH (GSPH) algorithm
/// GSPH uses Riemann solver for shock capturing, not artificial viscosity
struct GSPHTag {};

} // namespace sph

#endif // SPH_SIMULATION_ALGORITHM_TAGS_HPP

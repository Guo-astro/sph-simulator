/**
 * @file parameter_estimator.cpp
 * @brief Implementation of parameter estimation from particle configuration
 */

#include "core/parameters/parameter_estimator.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <limits>

namespace sph {

std::pair<real, real> ParameterEstimator::suggest_cfl(
    real particle_spacing,
    real sound_speed,
    real max_acceleration
) {
    // CFL coefficients from stability analysis (Monaghan 1989, Morris 1997)
    // These are NOT arbitrary - they come from von Neumann stability analysis
    
    // Sound-based CFL: ensures waves don't propagate more than h per timestep
    // From stability analysis: dt_sound = CFL_sound * h / (c_s + |v_max|)
    // Literature values: 0.25-0.4 for standard SPH
    // We use conservative 0.3 as baseline (Monaghan 2005)
    real cfl_sound = 0.3;
    
    // Force-based CFL: ensures acceleration doesn't cause large position changes
    // From stability analysis: dt_force = CFL_force * sqrt(h / |a_max|)
    // Literature values: 0.25 (Monaghan 1989), 0.125 (very conservative)
    // We use 0.25 as baseline
    real cfl_force = 0.25;
    
    // Adjust for numerical scheme stability
    // High-order schemes (GSPH, DISPH) can tolerate slightly higher CFL
    // Low-order schemes need more conservative values
    
    // Adjust for problem characteristics:
    
    // 1. Resolution effects
    //    Fine resolution → truncation error small → can use higher CFL
    //    Coarse resolution → truncation error large → need lower CFL
    if (particle_spacing < 0.001) {
        // Very fine resolution (dx < 0.001)
        cfl_sound = 0.35;  // Can be slightly more aggressive
        cfl_force = 0.3;
    } else if (particle_spacing > 0.1) {
        // Coarse resolution (dx > 0.1)
        cfl_sound = 0.2;   // Need to be more conservative
        cfl_force = 0.15;
    }
    
    // 2. Compressibility effects
    //    High Mach number flows have stronger shocks → need smaller CFL
    //    Mach number = |v| / c_s, but we don't have velocity here
    //    Use sound speed as proxy: high c_s often means compressible flow
    if (sound_speed > 10.0) {
        cfl_sound *= 0.75;  // Reduce for highly compressible flows
    } else if (sound_speed < 0.1) {
        cfl_sound *= 0.9;   // Slightly reduce for near-incompressible
    }
    
    // 3. Force magnitude effects
    //    Strong accelerations → rapid changes → need smaller timestep
    if (max_acceleration > 100.0) {
        cfl_force *= 0.7;   // Significant reduction for strong forces
    } else if (max_acceleration > 10.0) {
        cfl_force *= 0.85;  // Moderate reduction
    }
    
    // Safety bounds - never exceed established stable limits
    // These are from decades of SPH literature
    cfl_sound = std::min(cfl_sound, 0.4);   // Upper limit from Morris 1997
    cfl_sound = std::max(cfl_sound, 0.1);   // Lower limit (too small = inefficient)
    
    cfl_force = std::min(cfl_force, 0.3);   // Upper limit
    cfl_force = std::max(cfl_force, 0.05);  // Lower limit
    
    return {cfl_sound, cfl_force};
}

int ParameterEstimator::suggest_neighbor_number(
    real particle_spacing,
    real kernel_support,
    int dimension
) {
    // Calculate expected neighbors based on kernel support and dimension
    // 
    // Theory: For isotropic spacing dx, smoothing length h = kernel_support * dx
    // Number of neighbors = (kernel volume) / (particle volume)
    // 
    // 1D: 2*h / dx = 2 * kernel_support
    // 2D: π*h² / dx² = π * kernel_support²
    // 3D: (4/3)*π*h³ / dx³ = (4/3)*π * kernel_support³
    //
    // For anisotropic distributions, use geometric mean spacing to get
    // a representative "isotropic equivalent" neighbor count.
    // This prevents catastrophically large smoothing lengths when particles
    // are much more closely spaced in some dimensions than others.
    
    int neighbor_num = 0;
    
    // Use conservative safety factor of 1.2× theoretical value
    // (Original code used 4.0×, which was far too aggressive for anisotropic distributions)
    // Lower values work well when geometric mean spacing accounts for anisotropy
    constexpr real SAFETY_FACTOR = 1.2;
    
    if (dimension == 1) {
        // In 1D: neighbors within h on each side
        neighbor_num = static_cast<int>(2.0 * kernel_support * SAFETY_FACTOR);
    } else if (dimension == 2) {
        // In 2D: πh² / dx²
        real theoretical = M_PI * kernel_support * kernel_support;
        neighbor_num = static_cast<int>(theoretical * SAFETY_FACTOR);
    } else if (dimension == 3) {
        // In 3D: (4/3)πh³ / dx³
        real theoretical = (4.0 / 3.0) * M_PI * kernel_support * kernel_support * kernel_support;
        neighbor_num = static_cast<int>(theoretical * SAFETY_FACTOR);
    }
    
    // Clamp to reasonable range
    // Lower bounds ensure sufficient accuracy
    // Upper bounds prevent excessive computation
    const int min_safe = dimension == 1 ? 4 : dimension == 2 ? 12 : 30;
    const int max_reasonable = dimension == 1 ? 10 : dimension == 2 ? 50 : 100;
    
    neighbor_num = std::max(min_safe, std::min(max_reasonable, neighbor_num));
    
    return neighbor_num;
}

std::string ParameterEstimator::generate_rationale(
    const ParticleConfig& config,
    const ParameterSuggestions& suggestions
) {
    std::ostringstream msg;
    
    msg << "Parameter suggestions from SPH stability analysis:\n\n";
    
    msg << "=== Particle Configuration ===\n";
    msg << "  Spacing: " << config.avg_spacing << "\n";
    msg << "  Max sound speed: " << config.max_sound_speed << "\n";
    msg << "  Max acceleration: " << config.max_acceleration << "\n";
    msg << "  Dimension: " << config.dimension << "D\n\n";
    
    msg << "=== CFL Coefficients (von Neumann Stability) ===\n";
    msg << "  CFL_sound = " << suggestions.cfl_sound << "\n";
    msg << "    Formula: dt_sound = CFL_sound * h / (c_s + |v|)\n";
    msg << "    Physical meaning: wave doesn't propagate > h per timestep\n";
    msg << "    Literature: 0.25-0.4 (Monaghan 2005)\n\n";
    
    msg << "  CFL_force = " << suggestions.cfl_force << "\n";
    msg << "    Formula: dt_force = CFL_force * sqrt(h / |a|)\n";
    msg << "    Physical meaning: acceleration doesn't cause large displacement\n";
    msg << "    Literature: 0.125-0.25 (Monaghan 1989, Morris 1997)\n\n";
    
    msg << "=== Adjustments Applied ===\n";
    
    // Explain CFL adjustments
    if (config.avg_spacing < 0.001) {
        msg << "  ✓ Fine resolution (dx < 0.001) → increased CFL slightly\n";
    } else if (config.avg_spacing > 0.1) {
        msg << "  ✓ Coarse resolution (dx > 0.1) → reduced CFL for stability\n";
    }
    
    if (config.max_sound_speed > 10.0) {
        msg << "  ✓ High sound speed (c > 10) → reduced CFL_sound by 25%\n";
    } else if (config.max_sound_speed < 0.1) {
        msg << "  ✓ Nearly incompressible → slightly reduced CFL_sound\n";
    }
    
    if (config.max_acceleration > 100.0) {
        msg << "  ✓ Strong forces (|a| > 100) → reduced CFL_force by 30%\n";
    } else if (config.max_acceleration > 10.0) {
        msg << "  ✓ Moderate forces (|a| > 10) → reduced CFL_force by 15%\n";
    }
    
    msg << "\n=== Neighbor Number ===\n";
    msg << "  neighbor_number = " << suggestions.neighbor_number << "\n";
    msg << "  Calculated from kernel support volume in " << config.dimension << "D\n";
    msg << "  Ensures smoothing length captures sufficient particles for accuracy\n";
    
    return msg.str();
}

} // namespace sph

/**
 * @file parameter_estimator.tpp
 * @brief Template implementation of parameter estimation from particle configuration
 */

#pragma once

#include "core/parameter_estimator.hpp"
#include "core/vector.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <limits>

namespace sph {

template<int Dim>
ParticleConfig ParameterEstimator::analyze_particle_config(
    const std::vector<SPHParticle<Dim>>& particles
) {
    ParticleConfig config;
    
    if (particles.empty()) {
        config.min_spacing = 0.0;
        config.avg_spacing = 0.0;
        config.max_sound_speed = 0.0;
        config.max_velocity = 0.0;
        config.max_acceleration = 0.0;
        config.dimension = Dim;
        return config;
    }
    
    // Calculate spacing
    config.min_spacing = calculate_spacing_1d(particles);
    config.avg_spacing = config.min_spacing;  // Simplification
    
    // Find maximum sound speed
    config.max_sound_speed = 0.0;
    for (const auto& p : particles) {
        if (p.sound > config.max_sound_speed) {
            config.max_sound_speed = p.sound;
        }
    }
    
    // Find maximum velocity
    config.max_velocity = 0.0;
    for (const auto& p : particles) {
        real vel_mag = abs(p.vel);
        
        if (vel_mag > config.max_velocity) {
            config.max_velocity = vel_mag;
        }
    }
    
    // Find maximum acceleration
    config.max_acceleration = 0.0;
    for (const auto& p : particles) {
        real acc_mag = abs(p.acc);
        
        if (acc_mag > config.max_acceleration) {
            config.max_acceleration = acc_mag;
        }
    }
    
    // Estimate effective dimension
    config.dimension = estimate_dimension(particles);
    
    return config;
}

template<int Dim>
ParameterSuggestions ParameterEstimator::suggest_parameters(
    const std::vector<SPHParticle<Dim>>& particles,
    real kernel_support
) {
    ParameterSuggestions suggestions;
    
    // Analyze configuration
    ParticleConfig config = analyze_particle_config(particles);
    
    // Suggest CFL coefficients
    std::pair<real, real> cfl_pair = suggest_cfl(
        config.avg_spacing,
        config.max_sound_speed,
        config.max_acceleration
    );
    suggestions.cfl_sound = cfl_pair.first;
    suggestions.cfl_force = cfl_pair.second;
    
    // Suggest neighbor number
    suggestions.neighbor_number = suggest_neighbor_number(
        config.avg_spacing,
        kernel_support,
        config.dimension
    );
    
    // Generate rationale
    suggestions.rationale = generate_rationale(config, suggestions);
    
    return suggestions;
}

template<int Dim>
real ParameterEstimator::calculate_spacing_1d(const std::vector<SPHParticle<Dim>>& particles) {
    if (particles.size() < 2) {
        return 0.0;
    }
    
    // For anisotropic distributions, we need to find the characteristic spacing
    // that should be used for isotropic smoothing length calculation.
    // Using minimum spacing can be catastrophic if particles are much more closely
    // spaced in one dimension than others (e.g., dx=0.005, dy=0.05).
    //
    // Strategy: Find minimum spacing in each dimension separately, then use
    // geometric mean to get a representative isotropic spacing.
    // This prevents smoothing lengths from becoming catastrophically large.
    
    Vector<Dim> min_spacing_per_dim;
    for (int d = 0; d < Dim; ++d) {
        min_spacing_per_dim[d] = std::numeric_limits<real>::max();
    }
    
    // Sample to avoid O(N²) complexity
    const int sample_size = std::min(100, static_cast<int>(particles.size()));
    
    for (int i = 0; i < sample_size; ++i) {
        for (int j = i + 1; j < sample_size; ++j) {
            Vector<Dim> dx = particles[i].pos - particles[j].pos;
            
            // Track minimum spacing in each dimension
            for (int d = 0; d < Dim; ++d) {
                real abs_dx = std::abs(dx[d]);
                if (abs_dx > 1.0e-10 && abs_dx < min_spacing_per_dim[d]) {
                    min_spacing_per_dim[d] = abs_dx;
                }
            }
        }
    }
    
    // Calculate geometric mean of spacing across dimensions
    // This gives a representative "isotropic equivalent" spacing
    // For isotropic distributions, this equals the actual spacing
    // For anisotropic distributions, it prevents catastrophically large smoothing lengths
    real spacing_product = 1.0;
    int valid_dims = 0;
    for (int d = 0; d < Dim; ++d) {
        if (min_spacing_per_dim[d] < std::numeric_limits<real>::max()) {
            spacing_product *= min_spacing_per_dim[d];
            ++valid_dims;
        }
    }
    
    if (valid_dims == 0) {
        return 0.0;
    }
    
    // Geometric mean: (dx * dy * dz)^(1/Dim)
    real geometric_mean = std::pow(spacing_product, 1.0 / valid_dims);
    
    return geometric_mean;
}

template<int Dim>
int ParameterEstimator::estimate_dimension(const std::vector<SPHParticle<Dim>>& particles) {
    // This is a simple heuristic - could be improved
    // Check spread in each direction
    
    if (particles.empty()) {
        return Dim;
    }
    
    Vector<Dim> min_pos, max_pos;
    for (int d = 0; d < Dim; ++d) {
        min_pos[d] = std::numeric_limits<real>::max();
        max_pos[d] = -std::numeric_limits<real>::max();
    }
    
    for (const auto& p : particles) {
        for (int d = 0; d < Dim; ++d) {
            if (p.pos[d] < min_pos[d]) min_pos[d] = p.pos[d];
            if (p.pos[d] > max_pos[d]) max_pos[d] = p.pos[d];
        }
    }
    
    // Count dimensions with significant spread
    int effective_dim = 0;
    real threshold = 1.0e-6;
    
    for (int d = 0; d < Dim; ++d) {
        if (max_pos[d] - min_pos[d] > threshold) {
            ++effective_dim;
        }
    }
    
    return effective_dim > 0 ? effective_dim : Dim;
}

} // namespace sph

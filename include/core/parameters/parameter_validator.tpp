/**
 * @file parameter_validator.tpp
 * @brief Template implementation of parameter validation against particle configuration
 */

#pragma once

#include "core/parameters/parameter_validator.hpp"
#include "core/utilities/vector.hpp"
#include "exception.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <limits>

namespace sph {

template<int Dim>
void ParameterValidator::validate_cfl(
    const std::vector<SPHParticle<Dim>>& particles,
    real cfl_sound,
    real cfl_force
) {
    if (particles.empty()) {
        THROW_ERROR("Cannot validate CFL with empty particle list");
    }
    
    // Calculate characteristic scales
    real min_h = std::numeric_limits<real>::max();
    real max_c = 0.0;
    real max_a = 0.0;
    
    for (const auto& p : particles) {
        if (p.sml > 0.0 && p.sml < min_h) {
            min_h = p.sml;
        }
        
        if (p.sound > max_c) {
            max_c = p.sound;
        }
        
        real acc_mag = abs(p.acc);
        if (acc_mag > max_a) {
            max_a = acc_mag;
        }
    }
    
    // Calculate estimated timesteps
    real dt_sound_min = (max_c > 0.0) ? cfl_sound * min_h / max_c : 
                                        std::numeric_limits<real>::max();
    real dt_force_min = (max_a > 0.0) ? cfl_force * std::sqrt(min_h / max_a) : 
                                        std::numeric_limits<real>::max();
    
    // Validation thresholds
    const real min_acceptable_dt = 1.0e-10;  // Too small → simulation will be very slow
    const real max_safe_cfl_sound = 0.6;     // Above this risks instability
    const real max_safe_cfl_force = 0.3;
    
    std::ostringstream error_msg;
    bool has_error = false;
    
    // Check if CFL values are too aggressive
    if (cfl_sound > max_safe_cfl_sound) {
        has_error = true;
        error_msg << "CFL sound coefficient (" << cfl_sound << ") exceeds safe limit (" 
                  << max_safe_cfl_sound << "). ";
    }
    
    if (cfl_force > max_safe_cfl_force) {
        has_error = true;
        error_msg << "CFL force coefficient (" << cfl_force << ") exceeds safe limit (" 
                  << max_safe_cfl_force << "). ";
    }
    
    // Check if resulting timestep is unreasonably small
    if (dt_sound_min < min_acceptable_dt) {
        has_error = true;
        error_msg << "Sound-based timestep (" << dt_sound_min 
                  << ") is too small (h_min=" << min_h << ", c_max=" << max_c << "). "
                  << "Consider: (1) reducing cfl_sound, (2) using coarser resolution, "
                  << "or (3) reducing sound speeds. ";
    }
    
    if (max_a > 0.0 && dt_force_min < min_acceptable_dt) {
        has_error = true;
        error_msg << "Force-based timestep (" << dt_force_min 
                  << ") is too small (h_min=" << min_h << ", a_max=" << max_a << "). "
                  << "Consider: (1) reducing cfl_force, (2) using coarser resolution, "
                  << "or (3) reducing accelerations. ";
    }
    
    // Warn if timestep constraints are very different (factor > 100)
    if (dt_sound_min > 0.0 && dt_force_min > 0.0) {
        real ratio = std::max(dt_sound_min, dt_force_min) / 
                     std::min(dt_sound_min, dt_force_min);
        if (ratio > 100.0) {
            error_msg << "Sound and force timesteps differ by factor of " << ratio 
                      << ". One constraint may be unnecessarily restrictive. ";
        }
    }
    
    if (has_error) {
        error_msg << "\nConfiguration: h_min=" << min_h 
                  << ", c_max=" << max_c 
                  << ", a_max=" << max_a
                  << "\nResulting dt_sound=" << dt_sound_min
                  << ", dt_force=" << dt_force_min;
        
        throw std::runtime_error("CFL validation failed: " + error_msg.str());
    }
}

template<int Dim>
void ParameterValidator::validate_neighbor_number(
    const std::vector<SPHParticle<Dim>>& particles,
    int neighbor_number,
    real kernel_support
) {
    if (particles.empty()) {
        THROW_ERROR("Cannot validate neighbor_number with empty particle list");
    }
    
    if (neighbor_number <= 0) {
        THROW_ERROR("Neighbor number must be positive");
    }
    
    // Minimum recommended neighbors for each dimension
    const int min_neighbors_1d = 4;
    const int min_neighbors_2d = 12;
    const int min_neighbors_3d = 30;
    
    int min_recommended = Dim == 1 ? min_neighbors_1d :
                         Dim == 2 ? min_neighbors_2d :
                                    min_neighbors_3d;
    
    // Check if too few neighbors
    if (neighbor_number < min_recommended) {
        std::ostringstream msg;
        msg << "Neighbor number (" << neighbor_number << ") is below recommended minimum ("
            << min_recommended << ") for " << Dim << "D simulations. "
            << "This may result in poor accuracy and unphysical behavior.";
        throw std::runtime_error(msg.str());
    }
    
    // Check if neighbor number exceeds particle count
    if (neighbor_number > static_cast<int>(particles.size())) {
        std::ostringstream msg;
        msg << "Neighbor number (" << neighbor_number << ") exceeds total particle count ("
            << particles.size() << "). This is physically impossible.";
        throw std::runtime_error(msg.str());
    }
    
    // Estimate actual neighbors for sample particles
    const int sample_count = std::min(10, static_cast<int>(particles.size()));
    int total_actual = 0;
    
    for (int i = 0; i < sample_count; ++i) {
        int idx = i * particles.size() / sample_count;
        int actual = estimate_actual_neighbors(particles, idx, kernel_support);
        total_actual += actual;
    }
    
    int avg_actual = total_actual / sample_count;
    
    // Warn if mismatch between expected and actual
    if (std::abs(neighbor_number - avg_actual) > neighbor_number / 2) {
        std::ostringstream msg;
        msg << "Neighbor number (" << neighbor_number << ") differs significantly from "
            << "estimated actual neighbors (" << avg_actual << "). "
            << "This indicates a mismatch between particle spacing and expected neighbor count. "
            << "Consider adjusting neighbor_number or particle resolution.";
        // This is a warning, not a hard error
        // In production, could use a logger instead of throwing
    }
}

template<int Dim>
void ParameterValidator::validate_all(
    const std::vector<SPHParticle<Dim>>& particles,
    std::shared_ptr<SPHParameters> params
) {
    if (!params) {
        THROW_ERROR("Cannot validate null parameters");
    }
    
    // Validate CFL coefficients
    validate_cfl(particles, params->cfl.sound, params->cfl.force);
    
    // Validate neighbor number
    // Determine kernel support from kernel type
    real kernel_support = 2.0;  // Default for cubic spline
    
    validate_neighbor_number(
        particles, 
        params->physics.neighbor_number, 
        kernel_support
    );
    
    // Additional validations can be added here
    // e.g., check that smoothing lengths are reasonable
}

template<int Dim>
real ParameterValidator::calculate_min_spacing(const std::vector<SPHParticle<Dim>>& particles) {
    if (particles.size() < 2) {
        return 0.0;
    }
    
    real min_dist = std::numeric_limits<real>::max();
    
    // Sample pairs to estimate minimum spacing
    const int sample_size = std::min(100, static_cast<int>(particles.size()));
    
    for (int i = 0; i < sample_size; ++i) {
        const auto& p1 = particles[i];
        
        for (int j = i + 1; j < sample_size; ++j) {
            const auto& p2 = particles[j];
            
            Vector<Dim> dx = p1.pos - p2.pos;
            real dist = abs(dx);
            
            if (dist > 0.0 && dist < min_dist) {
                min_dist = dist;
            }
        }
    }
    
    return min_dist;
}

template<int Dim>
real ParameterValidator::calculate_max_sound_speed(const std::vector<SPHParticle<Dim>>& particles) {
    real max_c = 0.0;
    
    for (const auto& p : particles) {
        if (p.sound > max_c) {
            max_c = p.sound;
        }
    }
    
    return max_c;
}

template<int Dim>
real ParameterValidator::calculate_max_acceleration(const std::vector<SPHParticle<Dim>>& particles) {
    real max_a = 0.0;
    
    for (const auto& p : particles) {
        real acc_mag = abs(p.acc);
        
        if (acc_mag > max_a) {
            max_a = acc_mag;
        }
    }
    
    return max_a;
}

template<int Dim>
int ParameterValidator::estimate_actual_neighbors(
    const std::vector<SPHParticle<Dim>>& particles,
    int particle_idx,
    real kernel_support
) {
    const auto& p_i = particles[particle_idx];
    real search_radius = p_i.sml * kernel_support;
    int count = 0;
    
    for (size_t j = 0; j < particles.size(); ++j) {
        if (static_cast<int>(j) == particle_idx) continue;
        
        const auto& p_j = particles[j];
        
        Vector<Dim> dx = p_i.pos - p_j.pos;
        real dist = abs(dx);
        
        if (dist < search_radius) {
            ++count;
        }
    }
    
    return count;
}

} // namespace sph

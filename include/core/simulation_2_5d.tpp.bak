#pragma once

#include "simulation_2_5d.hpp"
#include "../parameters.hpp"
#include "../exception.hpp"

namespace sph {

Simulation25D::Simulation25D(std::shared_ptr<SPHParameters> param) {
    // Initialize 2D kernel for hydrodynamics
    kernel = std::make_shared<Cubic25D>();
    
    // Initialize 3D tree for gravity
    tree = std::make_shared<BHTree25D>();
    tree->initialize(param);
    
    time = param->time.start;
    dt = 0.0;
}

void Simulation25D::update_time() {
    time += dt;
}

void Simulation25D::make_tree() {
    tree->make(particles, particle_num);
}

void Simulation25D::add_scalar_array(const std::vector<std::string>& names) {
    const int num = particle_num;
    for (const auto& name : names) {
        additional_scalar_array[name].resize(num);
    }
}

void Simulation25D::add_vector_array(const std::vector<std::string>& names) {
    const int num = particle_num;
    for (const auto& name : names) {
        additional_vector_array[name].resize(num);
    }
}

std::vector<real>& Simulation25D::get_scalar_array(const std::string& name) {
    auto it = additional_scalar_array.find(name);
    if (it != additional_scalar_array.end()) {
        return it->second;
    } else {
        THROW_ERROR("additional_scalar_array does not have ", name);
    }
}

std::vector<Vector<2>>& Simulation25D::get_vector_array(const std::string& name) {
    auto it = additional_vector_array.find(name);
    if (it != additional_vector_array.end()) {
        return it->second;
    } else {
        THROW_ERROR("additional_vector_array does not have ", name);
    }
}

void Simulation25D::update_gravity_positions(const std::vector<real>& azimuthal_angles) {
    if (azimuthal_angles.empty()) {
        // Default: all particles at phi = 0
        for (auto& p : particles) {
            p.update_gravity_position(0.0);
        }
    } else {
        // Use provided azimuthal angles
        for (size_t i = 0; i < particles.size(); ++i) {
            real phi = (i < azimuthal_angles.size()) ? azimuthal_angles[i] : 0.0;
            particles[i].update_gravity_position(phi);
        }
    }
}

void Simulation25D::calculate_gravity() {
    // Update 3D positions for gravity calculations
    update_gravity_positions();
    
    // Rebuild gravity tree with updated positions
    make_tree();
    
    // Calculate gravitational forces for all particles
    for (auto& p : particles) {
        tree->tree_force(p);
    }
}

void Simulation25D::calculate_hydrodynamics() {
    // 2D hydrodynamic calculations using r-z coordinates
    // This would include:
    // - Density calculation
    // - Pressure calculation
    // - Viscosity forces
    // - Artificial viscosity
    // - etc.
    
    // Implementation would be similar to standard SPH but in 2D coordinates
    // using the 2D kernel for neighbor searches and force calculations
    
    // For now, this is a placeholder - actual implementation would depend
    // on the specific hydrodynamic algorithms used in the simulation
}

void Simulation25D::timestep() {
    // 1. Calculate gravitational forces (3D)
    calculate_gravity();
    
    // 2. Calculate hydrodynamic forces (2D)
    calculate_hydrodynamics();
    
    // 3. Time integration
    // Update positions and velocities using calculated accelerations
    
    // 4. Update time
    update_time();
}

} // namespace sph</content>
<parameter name="filePath">/Users/guo/sph-simulation/include/core/simulation_2_5d.tpp
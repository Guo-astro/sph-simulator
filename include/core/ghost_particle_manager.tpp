#ifndef SPH_GHOST_PARTICLE_MANAGER_TPP
#define SPH_GHOST_PARTICLE_MANAGER_TPP

#include "ghost_particle_manager.hpp"
#include <cmath>
#include <set>

namespace sph {

template<int Dim>
void GhostParticleManager<Dim>::initialize(const BoundaryConfiguration<Dim>& config) {
    config_ = config;
    clear();
}

template<int Dim>
void GhostParticleManager<Dim>::set_kernel_support_radius(real radius) {
    kernel_support_radius_ = radius;
}

template<int Dim>
void GhostParticleManager<Dim>::generate_ghosts(const std::vector<SPHParticle<Dim>>& real_particles) {
    clear();
    
    if (!config_.is_valid || real_particles.empty()) {
        return;
    }
    
    // Generate ghosts dimension by dimension
    // For each dimension, we generate ghosts from real particles only
    // This avoids complications with multi-dimensional corners
    
    for (int dim = 0; dim < Dim; ++dim) {
        switch (config_.types[dim]) {
            case BoundaryType::PERIODIC:
                generate_periodic_ghosts(real_particles, dim);
                break;
                
            case BoundaryType::MIRROR:
                if (config_.enable_lower[dim]) {
                    generate_mirror_ghosts(real_particles, dim, false);
                }
                if (config_.enable_upper[dim]) {
                    generate_mirror_ghosts(real_particles, dim, true);
                }
                break;
                
            case BoundaryType::NONE:
            case BoundaryType::FREE_SURFACE:
            default:
                // No ghost particles for these types
                break;
        }
    }
    
    // For multi-dimensional periodic boundaries, handle corners/edges
    if constexpr (Dim >= 2) {
        generate_corner_ghosts(real_particles);
    }
}

template<int Dim>
void GhostParticleManager<Dim>::update_ghosts(const std::vector<SPHParticle<Dim>>& real_particles) {
    // Check if any dimension uses mirror boundaries
    bool has_mirror = false;
    for (int d = 0; d < Dim; ++d) {
        if (config_.types[d] == BoundaryType::MIRROR) {
            has_mirror = true;
            break;
        }
    }
    
    // For mirror boundaries, we must regenerate ghosts to ensure proper velocity reflection
    // This is because velocity reflection depends on the current velocity of real particles
    if (has_mirror) {
        generate_ghosts(real_particles);
        return;
    }
    
    // For periodic boundaries, we can simply update properties without regenerating
    for (size_t i = 0; i < ghost_particles_.size(); ++i) {
        int real_idx = ghost_to_real_mapping_[i];
        if (real_idx >= 0 && real_idx < static_cast<int>(real_particles.size())) {
            // Copy properties from real particle
            ghost_particles_[i].vel = real_particles[real_idx].vel;
            ghost_particles_[i].dens = real_particles[real_idx].dens;
            ghost_particles_[i].pres = real_particles[real_idx].pres;
            ghost_particles_[i].mass = real_particles[real_idx].mass;
            ghost_particles_[i].sml = real_particles[real_idx].sml;
            ghost_particles_[i].ene = real_particles[real_idx].ene;
            
            // Note: Position is not updated - it was set during generation
        }
    }
}

template<int Dim>
const std::vector<SPHParticle<Dim>>& GhostParticleManager<Dim>::get_ghost_particles() const {
    return ghost_particles_;
}

template<int Dim>
int GhostParticleManager<Dim>::get_ghost_count() const {
    return static_cast<int>(ghost_particles_.size());
}

template<int Dim>
bool GhostParticleManager<Dim>::has_ghosts() const {
    return !ghost_particles_.empty();
}

template<int Dim>
void GhostParticleManager<Dim>::apply_periodic_wrapping(std::vector<SPHParticle<Dim>>& particles) {
    if (!config_.is_valid) {
        return;
    }
    
    for (auto& particle : particles) {
        for (int dim = 0; dim < Dim; ++dim) {
            if (config_.types[dim] == BoundaryType::PERIODIC) {
                const real range = config_.get_range(dim);
                
                if (particle.pos[dim] < config_.range_min[dim]) {
                    particle.pos[dim] += range;
                } else if (particle.pos[dim] > config_.range_max[dim]) {
                    particle.pos[dim] -= range;
                }
            }
        }
    }
}

template<int Dim>
const BoundaryConfiguration<Dim>& GhostParticleManager<Dim>::get_config() const {
    return config_;
}

template<int Dim>
void GhostParticleManager<Dim>::clear() {
    ghost_particles_.clear();
    ghost_to_real_mapping_.clear();
}

template<int Dim>
int GhostParticleManager<Dim>::get_source_particle_count() const {
    if (ghost_to_real_mapping_.empty()) {
        return 0;
    }
    // Count unique source particles
    std::set<int> unique_sources(ghost_to_real_mapping_.begin(), 
                                  ghost_to_real_mapping_.end());
    return static_cast<int>(unique_sources.size());
}

template<int Dim>
void GhostParticleManager<Dim>::generate_periodic_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles, 
    int dim) {
    
    const real range = config_.get_range(dim);
    
    for (size_t i = 0; i < real_particles.size(); ++i) {
        const auto& p = real_particles[i];
        
        // Check if particle is near lower boundary - create ghost at upper side
        if (is_near_boundary(p.pos, dim, false)) {
            SPHParticle<Dim> ghost = p;  // Copy all properties
            ghost.pos[dim] += range;       // Shift to upper side
            ghost.type = static_cast<int>(ParticleType::GHOST);  // Mark as ghost particle
            
            ghost_particles_.push_back(ghost);
            ghost_to_real_mapping_.push_back(static_cast<int>(i));
        }
        
        // Check if particle is near upper boundary - create ghost at lower side
        if (is_near_boundary(p.pos, dim, true)) {
            SPHParticle<Dim> ghost = p;  // Copy all properties
            ghost.pos[dim] -= range;       // Shift to lower side
            ghost.type = static_cast<int>(ParticleType::GHOST);  // Mark as ghost particle
            
            ghost_particles_.push_back(ghost);
            ghost_to_real_mapping_.push_back(static_cast<int>(i));
        }
    }
}

template<int Dim>
void GhostParticleManager<Dim>::generate_corner_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles) {
    
    // Handle corners/edges for multi-dimensional periodic boundaries
    // For 2D: corners (4 total)
    // For 3D: edges (12) and corners (8)
    
    // Count how many dimensions have periodic boundaries
    int periodic_dims[Dim];
    int num_periodic = 0;
    for (int d = 0; d < Dim; ++d) {
        if (config_.types[d] == BoundaryType::PERIODIC) {
            periodic_dims[num_periodic++] = d;
        }
    }
    
    if (num_periodic < 2) {
        return; // No corners/edges needed
    }
    
    // Generate corner/edge ghosts for particles near multiple boundaries
    for (size_t i = 0; i < real_particles.size(); ++i) {
        const auto& p = real_particles[i];
        
        // Check all combinations of periodic boundaries
        if (num_periodic == 2) {
            // 2D corners (or edges in 3D with 2 periodic dims)
            int d1 = periodic_dims[0];
            int d2 = periodic_dims[1];
            
            bool near_d1_lower = is_near_boundary(p.pos, d1, false);
            bool near_d1_upper = is_near_boundary(p.pos, d1, true);
            bool near_d2_lower = is_near_boundary(p.pos, d2, false);
            bool near_d2_upper = is_near_boundary(p.pos, d2, true);
            
            // Generate 4 corner ghosts if near both boundaries
            if (near_d1_lower && near_d2_lower) {
                SPHParticle<Dim> ghost = p;
                ghost.pos[d1] += config_.get_range(d1);
                ghost.pos[d2] += config_.get_range(d2);
                ghost.type = static_cast<int>(ParticleType::GHOST);
                ghost_particles_.push_back(ghost);
                ghost_to_real_mapping_.push_back(static_cast<int>(i));
            }
            if (near_d1_lower && near_d2_upper) {
                SPHParticle<Dim> ghost = p;
                ghost.pos[d1] += config_.get_range(d1);
                ghost.pos[d2] -= config_.get_range(d2);
                ghost.type = static_cast<int>(ParticleType::GHOST);
                ghost_particles_.push_back(ghost);
                ghost_to_real_mapping_.push_back(static_cast<int>(i));
            }
            if (near_d1_upper && near_d2_lower) {
                SPHParticle<Dim> ghost = p;
                ghost.pos[d1] -= config_.get_range(d1);
                ghost.pos[d2] += config_.get_range(d2);
                ghost.type = static_cast<int>(ParticleType::GHOST);
                ghost_particles_.push_back(ghost);
                ghost_to_real_mapping_.push_back(static_cast<int>(i));
            }
            if (near_d1_upper && near_d2_upper) {
                SPHParticle<Dim> ghost = p;
                ghost.pos[d1] -= config_.get_range(d1);
                ghost.pos[d2] -= config_.get_range(d2);
                ghost.type = static_cast<int>(ParticleType::GHOST);
                ghost_particles_.push_back(ghost);
                ghost_to_real_mapping_.push_back(static_cast<int>(i));
            }
        } else if (num_periodic == 3 && Dim == 3) {
            // 3D corners - all 8 combinations
            bool near_x_lower = is_near_boundary(p.pos, 0, false);
            bool near_x_upper = is_near_boundary(p.pos, 0, true);
            bool near_y_lower = is_near_boundary(p.pos, 1, false);
            bool near_y_upper = is_near_boundary(p.pos, 1, true);
            bool near_z_lower = is_near_boundary(p.pos, 2, false);
            bool near_z_upper = is_near_boundary(p.pos, 2, true);
            
            // Generate corner ghosts for all 8 corners
            for (int ix = 0; ix < 2; ++ix) {
                for (int iy = 0; iy < 2; ++iy) {
                    for (int iz = 0; iz < 2; ++iz) {
                        bool near_x = (ix == 0) ? near_x_lower : near_x_upper;
                        bool near_y = (iy == 0) ? near_y_lower : near_y_upper;
                        bool near_z = (iz == 0) ? near_z_lower : near_z_upper;
                        
                        // Need to be near at least 2 boundaries for corners
                        int boundary_count = (near_x ? 1 : 0) + (near_y ? 1 : 0) + (near_z ? 1 : 0);
                        if (boundary_count >= 2) {
                            SPHParticle<Dim> ghost = p;
                            if (near_x) ghost.pos[0] += (ix == 0) ? config_.get_range(0) : -config_.get_range(0);
                            if (near_y) ghost.pos[1] += (iy == 0) ? config_.get_range(1) : -config_.get_range(1);
                            if (near_z) ghost.pos[2] += (iz == 0) ? config_.get_range(2) : -config_.get_range(2);
                            ghost.type = static_cast<int>(ParticleType::GHOST);
                            ghost_particles_.push_back(ghost);
                            ghost_to_real_mapping_.push_back(static_cast<int>(i));
                        }
                    }
                }
            }
        }
    }
}

template<int Dim>
void GhostParticleManager<Dim>::generate_mirror_ghosts(
    const std::vector<SPHParticle<Dim>>& real_particles,
    int dim,
    bool is_upper) {
    
    for (size_t i = 0; i < real_particles.size(); ++i) {
        const auto& p = real_particles[i];
        
        // Check if particle is near this boundary
        if (is_near_boundary(p.pos, dim, is_upper)) {
            SPHParticle<Dim> ghost = p;  // Copy all properties
            
            // Mirror position across boundary
            ghost.pos = mirror_position(p.pos, dim, is_upper);
            
            // Reflect velocity according to mirror type
            if (config_.mirror_types[dim] == MirrorType::NO_SLIP) {
                reflect_velocity_no_slip(ghost.vel, dim);
            } else {
                reflect_velocity_free_slip(ghost.vel, dim);
            }
            
            ghost.type = static_cast<int>(ParticleType::GHOST);
            ghost_particles_.push_back(ghost);
            ghost_to_real_mapping_.push_back(static_cast<int>(i));
        }
    }
}

template<int Dim>
void GhostParticleManager<Dim>::reflect_velocity_no_slip(Vector<Dim>& velocity, int normal_dim) {
    // For no-slip: reflect ALL velocity components
    // This creates a stationary wall effect
    velocity = -velocity;
}

template<int Dim>
void GhostParticleManager<Dim>::reflect_velocity_free_slip(Vector<Dim>& velocity, int normal_dim) {
    // For free-slip: reflect only normal component, keep tangential
    // This allows particles to slide along the wall
    velocity[normal_dim] = -velocity[normal_dim];
}

template<int Dim>
bool GhostParticleManager<Dim>::is_near_boundary(
    const Vector<Dim>& position, 
    int dim, 
    bool is_upper) const {
    
    const real boundary_pos = is_upper ? config_.range_max[dim] : config_.range_min[dim];
    const real distance = is_upper ? 
        (boundary_pos - position[dim]) : 
        (position[dim] - boundary_pos);
    
    return distance < kernel_support_radius_ && distance >= 0.0;
}

template<int Dim>
Vector<Dim> GhostParticleManager<Dim>::mirror_position(
    const Vector<Dim>& position,
    int dim,
    bool is_upper) const {
    
    Vector<Dim> mirrored = position;
    const real boundary_pos = is_upper ? config_.range_max[dim] : config_.range_min[dim];
    
    // Mirror across boundary: new_pos = 2 * boundary - old_pos
    mirrored[dim] = 2.0 * boundary_pos - position[dim];
    
    return mirrored;
}

} // namespace sph

#endif // SPH_GHOST_PARTICLE_MANAGER_TPP

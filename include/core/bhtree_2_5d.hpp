#pragma once

#include "core/bhtree.hpp"
#include "sph_particle_2_5d.hpp"

namespace sph {

/**
 * @brief 2.5D Barnes-Hut Tree for gravity calculations
 * 
 * Uses 3D tree structure for gravitational interactions while
 * maintaining compatibility with 2D hydrodynamic particles.
 */
class BHTree25D {
private:
    // Use 3D BHTree for gravity calculations
    std::unique_ptr<BHTree<3>> gravity_tree_;
    
    // Parameters
    int max_level_;
    int leaf_particle_num_;
    bool is_periodic_;
    Vector<3> range_max_;
    Vector<3> range_min_;
    
    real g_constant_;
    real theta_;
    real theta2_;
    
public:
    BHTree25D() : gravity_tree_(std::make_unique<BHTree<3>>()) {}
    
    void initialize(std::shared_ptr<SPHParameters> param) {
        max_level_ = param->tree.max_level;
        leaf_particle_num_ = param->tree.leaf_particle_num;
        
        // Initialize 3D tree for gravity
        gravity_tree_->initialize(param);
        
        // Set up 3D periodic boundaries if needed
        is_periodic_ = param->periodic.is_valid;
        if (is_periodic_) {
            // Convert 2D periodic boundaries to 3D
            for (int i = 0; i < 2; ++i) {
                range_max_[i] = param->periodic.range_max[i];
                range_min_[i] = param->periodic.range_min[i];
            }
            // Azimuthal direction: assume 2π periodicity
            range_max_[2] = 2.0 * M_PI;
            range_min_[2] = 0.0;
        }
        
        if (param->gravity.is_valid) {
            g_constant_ = param->gravity.constant;
            theta_ = param->gravity.theta;
            theta2_ = theta_ * theta_;
        }
    }
    
    void resize(const int particle_num, const int tree_size = 5) {
        gravity_tree_->resize(particle_num, tree_size);
    }
    
    /**
     * @brief Build tree from 2.5D particles
     * Converts 2.5D particles to 3D for gravity calculations
     */
    void make(std::vector<SPHParticle25D>& particles_2_5d, const int particle_num) {
        // Convert 2.5D particles to 3D particles for gravity tree
        std::vector<SPHParticle<3>> particles_3d(particle_num);
        
        for (int i = 0; i < particle_num; ++i) {
            auto& p2d = particles_2_5d[i];
            auto& p3d = particles_3d[i];
            
            // Copy scalar properties
            p3d.id = p2d.id;
            p3d.mass = p2d.mass;
            p3d.sml = p2d.sml;
            
            // Use 3D gravity position
            p3d.pos = p2d.g_pos;
            
            // Update particle's gravity position if needed
            p2d.update_gravity_position();
        }
        
        // Build 3D tree
        gravity_tree_->make(particles_3d, particle_num);
        gravity_tree_->set_kernel();
    }
    
    /**
     * @brief Calculate gravitational forces for 2.5D particles
     */
    void tree_force(SPHParticle25D& p_i) {
        // Create temporary 3D particle for gravity calculation
        SPHParticle<3> p3d;
        p3d.id = p_i.id;
        p3d.mass = p_i.mass;
        p3d.sml = p_i.sml;
        p3d.pos = p_i.g_pos;
        p3d.phi = 0.0;
        p3d.acc = Vector<3>(0.0);
        
        // Calculate 3D gravity
        gravity_tree_->tree_force(p3d);
        
        // Store results back to 2.5D particle
        p_i.phi = p3d.phi;
        p_i.g_acc = p3d.acc;
        
        // Project 3D acceleration back to 2D hydro plane
        // For azimuthal symmetry, only radial and z components matter
        // g_acc[0] and g_acc[1] are in x-y plane, g_acc[2] is z
        // We need to convert back to cylindrical coordinates
        real r = p_i.r();
        if (r > 0.0) {
            // Radial component: projection of x-y acceleration onto radial direction
            real cos_phi = p_i.g_pos[0] / r;
            real sin_phi = p_i.g_pos[1] / r;
            p_i.acc[0] = p3d.acc[0] * cos_phi + p3d.acc[1] * sin_phi; // d(v_r)/dt
            p_i.acc[1] = p3d.acc[2]; // d(v_z)/dt
        } else {
            // At origin, only z-component
            p_i.acc[0] = 0.0;
            p_i.acc[1] = p3d.acc[2];
        }
    }
    
    /**
     * @brief Get 3D tree for advanced operations
     */
    BHTree<3>& get_3d_tree() { return *gravity_tree_; }
    const BHTree<3>& get_3d_tree() const { return *gravity_tree_; }
};

} // namespace sph</content>
<parameter name="filePath">/Users/guo/sph-simulation/include/core/bhtree_2_5d.hpp
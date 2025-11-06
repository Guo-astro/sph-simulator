#ifndef SPH_GHOST_PARTICLE_MANAGER_HPP
#define SPH_GHOST_PARTICLE_MANAGER_HPP

#include <vector>
#include <memory>
#include "../particles/sph_particle.hpp"
#include "../boundaries/boundary_types.hpp"
#include "../../defines.hpp"

namespace sph {

/**
 * @brief Manages ghost particles for boundary conditions in SPH simulations
 * 
 * This class implements the ghost particle method for handling boundary conditions
 * as described in Lajoie & Sills (2010). Ghost particles are auxiliary particles
 * that copy properties from real particles to ensure proper kernel interpolation
 * near boundaries.
 * 
 * Key features:
 * - Supports periodic boundaries (particles wrapped across domain)
 * - Supports mirror boundaries (particles reflected across walls)
 * - Dimension-agnostic (works for 1D, 2D, 3D)
 * - Efficient update mechanism (ghosts derived from real particles each timestep)
 * 
 * @tparam Dim Spatial dimensionality (1, 2, or 3)
 */
template<int Dim>
class GhostParticleManager {
private:
    BoundaryConfiguration<Dim> config_;
    std::vector<SPHParticle<Dim>> ghost_particles_;
    std::vector<int> ghost_to_real_mapping_;        ///< Maps ghost index to source real particle index
    real kernel_support_radius_;                    ///< Maximum distance for ghost particle generation
    
public:
    /**
     * @brief Default constructor
     */
    GhostParticleManager() : kernel_support_radius_(0.0) {}
    
    /**
     * @brief Initialize ghost particle manager with boundary configuration
     * 
     * @param config Boundary configuration specifying types and ranges
     */
    void initialize(const BoundaryConfiguration<Dim>& config);
    
    /**
     * @brief Set kernel support radius for ghost generation
     * 
     * Ghost particles are only created for real particles within this distance
     * from boundaries to optimize performance.
     * 
     * @param radius Kernel support radius (typically 2h or 3h)
     */
    void set_kernel_support_radius(real radius);
    
    /**
     * @brief Generate ghost particles from real particles
     * 
     * Creates ghost particles near boundaries according to boundary configuration.
     * Should be called after particle positions change significantly.
     * 
     * @param real_particles Vector of real SPH particles
     */
    void generate_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Update ghost particle properties from real particles
     * 
     * DEPRECATED: This method only updates properties but NOT positions.
     * Use regenerate_ghosts() instead to ensure ghost positions reflect
     * current particle positions (critical for density calculation).
     * 
     * @param real_particles Vector of real SPH particles
     */
    void update_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Regenerate ghost particles from current real particle positions
     * 
     * This is a declarative wrapper that clears existing ghosts and generates
     * new ones based on current particle positions. Should be called after
     * particles move (e.g., after predict() step in solver).
     * 
     * Ensures ghost particles always reflect the Morris 1997 formula:
     *   x_ghost = 2*x_wall - x_real
     * 
     * @param real_particles Vector of real SPH particles
     */
    void regenerate_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Get all ghost particles
     * 
     * @return Const reference to ghost particles vector
     */
    const std::vector<SPHParticle<Dim>>& get_ghost_particles() const;
    
    /**
     * @brief Get number of ghost particles
     */
    int get_ghost_count() const;
    
    /**
     * @brief Check if ghost particles exist
     */
    bool has_ghosts() const;
    
    /**
     * @brief Apply periodic boundary conditions to real particle positions
     * 
     * Wraps particles that have moved outside the domain back to the other side.
     * 
     * @param particles Vector of real particles to apply wrapping to
     */
    void apply_periodic_wrapping(std::vector<SPHParticle<Dim>>& particles);
    
    /**
     * @brief Get the boundary configuration
     */
    const BoundaryConfiguration<Dim>& get_config() const;
    
    /**
     * @brief Clear all ghost particles
     */
    void clear();
    
    /**
     * @brief Get number of real particles that contributed to ghosts
     */
    int get_source_particle_count() const;
    
private:
    /**
     * @brief Generate periodic ghost particles for a specific dimension
     * 
     * @param real_particles Source real particles
     * @param dim Dimension index (0=x, 1=y, 2=z)
     */
    void generate_periodic_ghosts(const std::vector<SPHParticle<Dim>>& real_particles, int dim);
    
    /**
     * @brief Generate corner/edge ghost particles for multi-dimensional periodic boundaries
     * 
     * Handles:
     * - 2D: 4 corners
     * - 3D with 2 periodic dims: 4 edges
     * - 3D with 3 periodic dims: 12 edges + 8 corners
     * 
     * @param real_particles Source real particles
     */
    void generate_corner_ghosts(const std::vector<SPHParticle<Dim>>& real_particles);
    
    /**
     * @brief Generate mirror ghost particles for a specific dimension and boundary
     * 
     * @param real_particles Source real particles
     * @param dim Dimension index (0=x, 1=y, 2=z)
     * @param is_upper Whether this is the upper boundary (false = lower)
     */
    void generate_mirror_ghosts(const std::vector<SPHParticle<Dim>>& real_particles, 
                                int dim, 
                                bool is_upper);
    
    /**
     * @brief Reflect velocity for no-slip boundary condition
     * 
     * @param velocity Velocity vector to reflect
     * @param normal_dim Dimension normal to the wall
     */
    void reflect_velocity_no_slip(Vector<Dim>& velocity, int normal_dim);
    
    /**
     * @brief Reflect velocity for free-slip boundary condition
     * 
     * @param velocity Velocity vector to reflect
     * @param normal_dim Dimension normal to the wall
     */
    void reflect_velocity_free_slip(Vector<Dim>& velocity, int normal_dim);
    
    /**
     * @brief Check if a particle is near a boundary
     * 
     * @param position Particle position
     * @param dim Dimension to check
     * @param is_upper Check upper boundary (false = lower)
     * @return True if particle is within kernel support radius of boundary
     */
    bool is_near_boundary(const Vector<Dim>& position, int dim, bool is_upper) const;
    
    /**
     * @brief Mirror a position across a boundary
     * 
     * @param position Position to mirror
     * @param dim Dimension to mirror across
     * @param is_upper Mirror across upper boundary (false = lower)
     * @return Mirrored position
     */
    Vector<Dim> mirror_position(const Vector<Dim>& position, int dim, bool is_upper) const;
};

} // namespace sph

// Include template implementation
#include "ghost_particle_manager.tpp"

#endif // SPH_GHOST_PARTICLE_MANAGER_HPP

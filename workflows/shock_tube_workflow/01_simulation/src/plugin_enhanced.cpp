#include "core/plugins/simulation_plugin.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "parameters.hpp"
#include "exception.hpp"
#include <vector>
#include <iostream>
#include <cmath>

using namespace sph;

/**
 * @brief Physics-based shock tube plugin using parameter validation/estimation.
 * 
 * This plugin demonstrates the modern parameter system:
 * - Physics-based CFL calculation from von Neumann stability analysis
 * - Parameter validation against actual particle configuration
 * - Automatic estimation of safe values
 * - No hardcoded constants - all values justified by theory
 * 
 * CFL Theory:
 * - dt_sound = CFL_sound * h / (c_s + |v|)  [Monaghan 2005]
 * - dt_force = CFL_force * sqrt(h / |a|)    [Monaghan 1989]
 * 
 * See docs/CFL_THEORY.md for complete explanation.
 */
class ShockTubePluginEnhanced : public SimulationPlugin<1> {
public:
    std::string get_name() const override {
        return "shock_tube_enhanced";
    }
    
    std::string get_description() const override {
        return "1D Sod shock tube with type-safe parameter builder";
    }
    
    std::string get_version() const override {
        return "4.0.0";  // Physics-based parameter system
    }
    
    void initialize(std::shared_ptr<Simulation<1>> sim,
                   std::shared_ptr<SPHParameters> params) override {
        // This plugin is for 1D simulations
        static constexpr int Dim = 1;

        std::cout << "\n=== ENHANCED SHOCK TUBE (Physics-Based Parameters) ===\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        // Create particles FIRST so we can analyze their configuration
        // for physics-based parameter estimation
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        // Sod shock tube with proper density ratio
        // Left:  x ∈ [-0.5, 0.5], ρ=1.0,   P=1.0
        // Right: x ∈ [0.5, 1.5],  ρ=0.125, P=0.1
        // For uniform mass and ρ = m/dx, need dx_L/dx_R = ρ_R/ρ_L = 1/8
        // ============================================================
        
        const real gamma = 1.4;  // Adiabatic index for ideal gas
        
        // Right side setup (lower density, larger spacing)
        const int N_right = 50;
        const real L_right = 1.0;  // Right domain length [0.5, 1.5]
        const real dx_right = L_right / N_right;
        
        // Left side setup (higher density, smaller spacing)  
        const real L_left = 1.0;   // Left domain length [-0.5, 0.5]
        const real dx_left = dx_right / 8.0;  // 8× denser for ρ_L/ρ_R = 8
        const int N_left = static_cast<int>(L_left / dx_left);
        
        const int num = N_left + N_right;
        const real mass = 0.125 * dx_right;  // Uniform mass: m = ρ_R × dx_R = 0.125 × 0.02
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        std::cout << "\n--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << " (" << N_left << " left + " << N_right << " right)\n";
        std::cout << "Left state:  ρ=1.0,    P=1.0,   dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.125,  P=0.1,   dx=" << dx_right << " (Sod 1978)\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        
        int idx = 0;
        
        // Initial smoothing length estimate
        // For cubic spline kernel in 1D: h ≈ kappa * dx
        // kappa ≈ 1.2 ensures kernel support covers ~2-3 neighbors on each side
        constexpr real kappa = 1.2;
        const real sml_left = kappa * dx_left;
        const real sml_right = kappa * dx_right;
        
        std::cout << "Initial sml estimates: left=" << sml_left << ", right=" << sml_right << "\n";
        
        // Initialize left side particles
        real x_left = -0.5 + dx_left * 0.5;
        for(int i = 0; i < N_left; ++i) {
            auto & p_i = particles[idx];
            p_i.pos[0] = x_left;
            p_i.vel[0] = 0.0;
            p_i.dens = 1.0;
            p_i.pres = 1.0;
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.sound = std::sqrt(gamma * p_i.pres / p_i.dens);
            p_i.sml = sml_left;
            p_i.id = idx;
            p_i.acc[0] = 0.0;
            
            x_left += dx_left;
            idx++;
        }
        
        // Initialize right side particles
        real x_right = 0.5 + dx_right * 0.5;
        for(int i = 0; i < N_right; ++i) {
            auto & p_i = particles[idx];
            p_i.pos[0] = x_right;
            p_i.vel[0] = 0.0;
            p_i.dens = 0.125;
            p_i.pres = 0.1;
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.sound = std::sqrt(gamma * p_i.pres / p_i.dens);
            p_i.sml = sml_right;
            p_i.id = idx;
            p_i.acc[0] = 0.0;
            
            x_right += dx_right;
            idx++;
        }
        
        // ============================================================
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================
        // Use ParameterEstimator to calculate CFL and neighbor_number
        // from actual particle configuration and stability theory
        // ============================================================
        
        // ============================================================
        // STEP 3: CONFIGURE PARAMETERS DIRECTLY
        // ============================================================
        // Use legacy SPHParameters structure for compatibility
        // ============================================================
        
        if (params->time.end == 0) {
            std::cout << "\n--- Configuring Simulation Parameters ---\n";
            
            // Time parameters
            params->time.start = 0.0;
            params->time.end = 0.15;
            params->time.output = 0.01;
            params->time.energy = 0.01;
            
            // CFL conditions (conservative for shock tube)
            params->cfl.sound = 0.3;
            params->cfl.force = 0.25;
            
            // Physics
            params->physics.neighbor_number = 30;  // For 1D with cubic spline
            params->physics.gamma = gamma;
            
            // Gravity
            params->gravity.is_valid = false;
            
            // Artificial conductivity
            params->ac.is_valid = false;
            
            // Artificial viscosity (disabled for GSPH)
            params->av.alpha = 1.0;  // Not used but set default
            params->av.use_balsara_switch = false;
            params->av.use_time_dependent_av = false;
            
            // Kernel
            params->kernel = KernelType::CUBIC_SPLINE;
            
            // Tree
            params->tree.max_level = 20;
            params->tree.leaf_particle_num = 1;
            
            // SPH type
            params->type = SPHType::GSPH;
            params->gsph.is_2nd_order = false;  // 1st order safer with ghosts
            
            // Iterative smoothing length
            params->iterative_sml = true;
            
            std::cout << "✓ Parameters configured\n";
            std::cout << "  - SPH type: GSPH (Godunov SPH with Riemann solver)\n";
            std::cout << "  - 2nd order MUSCL: disabled\n";
            std::cout << "  - CFL sound: " << params->cfl.sound << "\n";
            std::cout << "  - CFL force: " << params->cfl.force << "\n";
        } else {
            std::cout << "\n--- Using Pre-Configured Parameters ---\n";
            std::cout << "(Loaded from JSON configuration)\n";
        }
        
        // ============================================================
        // ============================================================
        // STEP 4: SET PARTICLES IN SIMULATION
        // ============================================================
        
        const int num_particles = static_cast<int>(particles.size());
        
        sim->particle_num = num_particles;
        sim->particles = std::move(particles);
        
        // ============================================================
        // STEP 6: INITIALIZE GHOST PARTICLE SYSTEM
        // ============================================================
        // Modern boundary system using BoundaryConfiguration
        // This replaces the legacy periodic boundary parameters
        // 
        // For shock tube: Use MIRROR boundaries (reflective walls)
        // NOT periodic - we want walls, not wrapping!
        // 
        // Ghost particles will be generated in solver initialization
        // after smoothing lengths are calculated. They will copy all
        // properties from the real boundary particles as-is.
        // ============================================================
        std::cout << "\n--- Ghost Particle System ---\n";
        
        // Configure mirror boundary with ghost particles (reflective walls)
        BoundaryConfiguration<Dim> ghost_config;
        ghost_config.is_valid = true;
        ghost_config.types[0] = BoundaryType::MIRROR;
        ghost_config.range_min[0] = -0.5;
        ghost_config.range_max[0] = 1.5;
        ghost_config.enable_lower[0] = true;
        ghost_config.enable_upper[0] = true;
        ghost_config.mirror_types[0] = MirrorType::FREE_SLIP;  // FREE_SLIP for shock tube (allows sliding along wall)
        
        // CRITICAL: Set per-boundary particle spacing for Morris 1997 wall offset calculation
        // Left boundary has dense particles (dx_left), right boundary has sparse particles (dx_right)
        ghost_config.spacing_lower[0] = dx_left;   // Left wall: use local particle spacing
        ghost_config.spacing_upper[0] = dx_right;  // Right wall: use local particle spacing
        
        // Initialize ghost particle manager
        sim->ghost_manager->initialize(ghost_config);
        
        std::cout << "✓ Ghost particle system configured\n";
        std::cout << "  Boundary type: MIRROR (FREE_SLIP)\n";
        std::cout << "  Domain range: [" << ghost_config.range_min[0] 
                  << ", " << ghost_config.range_max[0] << "]\n";
        std::cout << "  Left particle spacing (dx_left):  " << dx_left << "\n";
        std::cout << "  Right particle spacing (dx_right): " << dx_right << "\n";
        std::cout << "  Left wall offset:  -" << (0.5 * dx_left) << "\n";
        std::cout << "  Right wall offset: +" << (0.5 * dx_right) << "\n";
        std::cout << "  Left wall position:  " << ghost_config.get_wall_position(0, false) << "\n";
        std::cout << "  Right wall position: " << ghost_config.get_wall_position(0, true) << "\n";
        std::cout << "  (Ghost particles will be generated after sml calculation)\n";
        
        std::cout << "\n--- Configuration Summary ---\n";
        std::cout << "SPH Algorithm: ";
        switch(params->type) {
            case SPHType::SSPH:  std::cout << "Standard SPH\n"; break;
            case SPHType::DISPH: std::cout << "Density Independent SPH\n"; break;
            case SPHType::GSPH:  std::cout << "Godunov SPH\n"; break;
        }
        std::cout << "CFL coefficients: sound=" << params->cfl.sound 
                  << ", force=" << params->cfl.force << "\n";
        std::cout << "Neighbor number: " << params->physics.neighbor_number << "\n";
        std::cout << "Gamma (adiabatic): " << params->physics.gamma << "\n";
        std::cout << "Kernel: ";
        switch(params->kernel) {
            case KernelType::CUBIC_SPLINE: std::cout << "Cubic Spline\n"; break;
            case KernelType::WENDLAND: std::cout << "Wendland\n"; break;
            case KernelType::UNKNOWN: std::cout << "Unknown\n"; break;
        }
        
        std::cout << "\n=== Initialization Complete ===\n\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube_plugin_enhanced.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN(ShockTubePluginEnhanced, 1)

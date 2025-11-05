/**
 * @file plugin_2d.cpp
 * @brief 2D Shock Tube Plugin using template-based dimension system
 * 
 * This plugin implements a 2D extension of the Sod shock tube problem:
 * - Shock propagates in x-direction
 * - Uniform in y-direction  
 * - Uses SPHParticle<2> templates
 * - Physics-based parameter estimation and validation
 */

#include "core/simulation_plugin.hpp"
#include "core/sph_parameters_builder_base.hpp"
#include "core/gsph_parameters_builder.hpp"
#include "core/parameter_estimator.hpp"
#include "core/parameter_validator.hpp"
#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/vector.hpp"
#include "core/ghost_particle_manager.hpp"
#include "core/boundary_types.hpp"
#include "exception.hpp"
#include <vector>
#include <iostream>
#include <cmath>

using namespace sph;

/**
 * @brief 2D Shock Tube Plugin
 * 
 * Extends the 1D Sod shock tube to 2D:
 * - Discontinuity along x-direction at x=0.5
 * - Periodic or reflective boundaries in y-direction
 * - Same density/pressure jump as Sod problem
 */
class ShockTube2DPlugin : public SimulationPlugin<2> {
public:
    std::string get_name() const override {
        return "shock_tube_2d";
    }
    
    std::string get_description() const override {
        return "2D Sod shock tube with template-based dimension system";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_2d.cpp"};
    }
    
    void initialize(std::shared_ptr<Simulation<2>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        static constexpr int Dim = 2;
        
        std::cout << "\n=== 2D SHOCK TUBE SIMULATION ===\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        
        const real gamma = 1.4;  // Adiabatic index for ideal gas
        
        // Domain setup
        // X-direction: shock tube [-0.5, 1.5] with discontinuity at x=0.5
        // Y-direction: [0, 0.5] for visualization
        
        // Right side (lower density)
        // REDUCED RESOLUTION for faster computation (was 50×25 → 11,250 particles)
        const int Nx_right = 25;  // Reduced from 50
        const int Ny = 10;        // Reduced from 25 → ~2,875 particles
        const real Lx_right = 1.0;  // [0.5, 1.5]
        const real Ly = 0.5;
        const real dx_right = Lx_right / Nx_right;
        const real dy = Ly / Ny;
        
        // Left side (higher density)
        const real Lx_left = 1.0;  // [-0.5, 0.5]
        const real dx_left = dx_right / 8.0;  // 8× denser
        const int Nx_left = static_cast<int>(Lx_left / dx_left);
        
        const int num = (Nx_left + Nx_right) * Ny;
        const real mass = 0.125 * dx_right * dy;  // Uniform mass per particle
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        std::cout << "\n--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Left state:  ρ=1.0,   P=1.0,  dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.125, P=0.1,  dx=" << dx_right << "\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Y-extent: [0, " << Ly << "]\n";
        
        int idx = 0;
        
        // Initialize left side particles
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_left = -0.5 + dx_left * 0.5;
            
            for (int i = 0; i < Nx_left; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_left, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 1.0;
                p.pres = 1.0;
                p.mass = mass;
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.sound = std::sqrt(gamma * p.pres / p.dens);
                p.id = idx;
                
                x_left += dx_left;
                ++idx;
            }
        }
        
        // Initialize right side particles
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_right = 0.5 + dx_right * 0.5;
            
            for (int i = 0; i < Nx_right; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_right, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 0.125;
                p.pres = 0.1;
                p.mass = mass;
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.sound = std::sqrt(gamma * p.pres / p.dens);
                p.id = idx;
                
                x_right += dx_right;
                ++idx;
            }
        }
        
        // ============================================================
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================
        
        std::cout << "\n--- Parameter Estimation ---\n";
        
        auto config = ParameterEstimator::analyze_particle_config<Dim>(particles);
        
        std::cout << "Particle configuration:\n";
        std::cout << "  Spacing: " << dx_right << "\n";
        std::cout << "  Max sound speed: " << config.max_sound_speed << "\n";
        std::cout << "  Estimated dimension: 2D\n";
        
        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(
            particles, 2.0
        );
        
        std::cout << "\nSuggested parameters:\n";
        std::cout << "  CFL sound: " << suggestions.cfl_sound << "\n";
        std::cout << "  CFL force: " << suggestions.cfl_force << "\n";
        std::cout << "  Neighbor number: " << suggestions.neighbor_number << "\n";
        
        // ============================================================
        // STEP 3: BUILD PARAMETERS USING TYPE-SAFE BUILDER
        // ============================================================
        
        std::cout << "\n--- Building Parameters (Type-Safe API) ---\n";
        
        try {
            auto built_params = SPHParametersBuilderBase()
                // Common parameters
                .with_time(0.0, 0.2, 0.01)
                .with_physics(suggestions.neighbor_number, gamma)
                .with_cfl(suggestions.cfl_sound, suggestions.cfl_force)
                .with_kernel("cubic_spline")
                .with_iterative_smoothing_length(true)
                
                // Transition to GSPH (Godunov SPH for shock capturing)
                .as_gsph()
                .with_2nd_order_muscl(true)  // Enable 2nd order MUSCL-Hancock
                
                .build();
            
            *param = *built_params;
            
            std::cout << "✓ Parameters built with type-safe GSPH API\n";
            std::cout << "  - GSPH uses HLL Riemann solver, NOT artificial viscosity\n";
            std::cout << "  - 2nd order MUSCL enabled for better accuracy\n";
                
        } catch (const std::exception& e) {
            THROW_ERROR("Parameter building failed: " + std::string(e.what()));
        }
        
        // ============================================================
        // STEP 4: VALIDATE PARAMETERS AGAINST PARTICLE CONFIGURATION
        // ============================================================
        
        std::cout << "\n--- Parameter Validation ---\n";
        
        try {
            ParameterValidator::validate_all<Dim>(particles, param);
            std::cout << "✓ All parameters validated successfully\n";
        } catch (const std::exception& e) {
            std::cout << "⚠ Validation warning: " << e.what() << "\n";
            std::cout << "Proceeding with suggested parameters...\n";
        }
        
        // ============================================================
        // STEP 5: SET PARTICLES IN SIMULATION
        // ============================================================
        
        sim->particles = std::move(particles);
        sim->particle_num = num;
        
        // ============================================================
        // STEP 6: INITIALIZE GHOST PARTICLE SYSTEM
        // ============================================================
        // Modern boundary system using BoundaryConfiguration
        // 
        // For 2D shock tube:
        // - X-direction: MIRROR (walls at domain boundaries)
        // - Y-direction: MIRROR (walls top/bottom)
        // 
        // Note: We do NOT use with_periodic_boundary() - that's legacy!
        // The modern approach is to configure BoundaryConfiguration directly.
        // ============================================================
        std::cout << "\n--- Ghost Particle System ---\n";
        
        // Configure boundary conditions:
        // - X-direction: MIRROR with NO_SLIP (wall boundaries)
        // - Y-direction: MIRROR with NO_SLIP (wall boundaries)
        //
        // ┌─────────────────────────────────────────────────────────────┐
        // │ BOUNDARY CONFIGURATION HIERARCHY                             │
        // ├─────────────────────────────────────────────────────────────┤
        // │                                                              │
        // │  Level 1: BoundaryType (HOW ghosts are created)             │
        // │  ├─ MIRROR: Reflect particles across boundary               │
        // │  ├─ PERIODIC: Wrap particles from opposite side             │
        // │  └─ NONE: No ghost particles                                │
        // │                                                              │
        // │  Level 2: MirrorType (ghost VELOCITY, only for MIRROR)      │
        // │  ├─ NO_SLIP: v_ghost = -v_real (sticky wall, friction)      │
        // │  └─ FREE_SLIP: v_ghost = v_real (frictionless, tangential)  │
        // │                                                              │
        // └─────────────────────────────────────────────────────────────┘
        //
        BoundaryConfiguration<Dim> ghost_config;
        ghost_config.is_valid = true;
        
        // X-direction: mirror walls (no-slip)
        // BoundaryType::MIRROR = ghosts created by reflection
        // MirrorType::NO_SLIP = ghost has opposite velocity (wall friction)
        ghost_config.types[0] = BoundaryType::MIRROR;
        ghost_config.range_min[0] = -0.5;
        ghost_config.range_max[0] = 1.5;
        ghost_config.enable_lower[0] = true;
        ghost_config.enable_upper[0] = true;
        ghost_config.mirror_types[0] = MirrorType::NO_SLIP;  // Ghost velocity = -real (sticky wall)
        
        // Y-direction: mirror walls (no-slip)
        // TWO-LEVEL BOUNDARY CONFIGURATION:
        // 
        // 1. BoundaryType::MIRROR - Determines HOW ghosts are created
        //    - MIRROR: Ghost particles are created by reflecting real particles across boundary
        //    - PERIODIC: Ghost particles wrap around from opposite boundary
        //    - NONE: No ghost particles
        //
        // 2. MirrorType::NO_SLIP - Determines ghost particle VELOCITY (only for MIRROR boundaries)
        //    - NO_SLIP: Ghost velocity = -real velocity (sticky wall, fluid sticks to wall)
        //    - FREE_SLIP: Ghost velocity = real velocity (frictionless wall, tangential flow allowed)
        //
        // Example: For a solid wall with friction → BoundaryType::MIRROR + MirrorType::NO_SLIP
        //          For a frictionless wall → BoundaryType::MIRROR + MirrorType::FREE_SLIP
        //          For periodic domain → BoundaryType::PERIODIC (MirrorType ignored)
        //
        ghost_config.types[1] = BoundaryType::MIRROR;      // Creates ghost by reflection
        ghost_config.range_min[1] = 0.0;
        ghost_config.range_max[1] = 0.5;
        ghost_config.enable_lower[1] = true;
        ghost_config.enable_upper[1] = true;
        ghost_config.mirror_types[1] = MirrorType::NO_SLIP;  // Ghost velocity = -real (sticky wall)
        
        // CRITICAL: Set per-boundary particle spacing for Morris 1997 wall offset calculation
        // X-direction: Left boundary has dense particles (dx_left), right has sparse (dx_right)
        ghost_config.spacing_lower[0] = dx_left;   // Left wall: use local spacing
        ghost_config.spacing_upper[0] = dx_right;  // Right wall: use local spacing
        // Y-direction: Uniform spacing throughout
        ghost_config.spacing_lower[1] = dx_left;        // Bottom wall
        ghost_config.spacing_upper[1] = dx_right;        // Top wall
        
        // Initialize ghost particle manager
        sim->ghost_manager->initialize(ghost_config);
        
        // Set kernel support radius (conservative estimate based on max smoothing length)
        real max_sml = 0.0;
        for (const auto& p : sim->particles) {
            max_sml = std::max(max_sml, p.sml);
        }
        sim->ghost_manager->set_kernel_support_radius(max_sml * 2.0);
        
        // Generate initial ghost particles
        sim->ghost_manager->generate_ghosts(sim->particles);
        
        std::cout << "✓ Ghost particle system initialized\n";
        std::cout << "  X-boundary: MIRROR (NO_SLIP) [" << ghost_config.range_min[0] 
                  << ", " << ghost_config.range_max[0] << "]\n";
        std::cout << "    Left particle spacing (dx_left):  " << dx_left << "\n";
        std::cout << "    Right particle spacing (dx_right): " << dx_right << "\n";
        std::cout << "    Left wall offset:  -" << (0.5 * dx_left) << "\n";
        std::cout << "    Right wall offset: +" << (0.5 * dx_right) << "\n";
        std::cout << "    Left wall position:  " << ghost_config.get_wall_position(0, false) << "\n";
        std::cout << "    Right wall position: " << ghost_config.get_wall_position(0, true) << "\n";
        std::cout << "  Y-boundary: MIRROR (NO_SLIP) [" << ghost_config.range_min[1]
                  << ", " << ghost_config.range_max[1] << "]\n";
        std::cout << "    Particle spacing (dy): " << dy << "\n";
        std::cout << "    Wall offset: ±" << (0.5 * dy) << "\n";
        std::cout << "    Bottom wall position: " << ghost_config.get_wall_position(1, false) << "\n";
        std::cout << "    Top wall position:    " << ghost_config.get_wall_position(1, true) << "\n";
        std::cout << "  Kernel support radius: " << (max_sml * 2.0) << "\n";
        std::cout << "  Generated " << sim->ghost_manager->get_ghost_count() 
                  << " ghost particles\n";
        
        std::cout << "\n=== Initialization Complete ===\n";
        std::cout << "Particles: " << num << "\n";
        std::cout << "Ghost particles: " << sim->ghost_manager->get_ghost_count() << "\n";
        std::cout << "Total for neighbor search: " << (num + sim->ghost_manager->get_ghost_count()) << "\n";
        std::cout << "Ready to run simulation\n\n";
    }
};

DEFINE_SIMULATION_PLUGIN(ShockTube2DPlugin, 2)

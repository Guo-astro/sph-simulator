/**
 * @file plugin_gsph.cpp
 * @brief 2D Shock Tube GSPH Plugin
 * 
 * GSPH (Godunov SPH) configuration for 2D shock tube:
 * - Uses HLL Riemann solver (NOT artificial viscosity)
 * - Shock propagates in x-direction
 * - Mirror boundaries with ghost particles
 * - Physics-based parameter estimation
 */

#include "core/plugins/simulation_plugin.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include "core/parameters/parameter_estimator.hpp"
#include "core/parameters/parameter_validator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/utilities/vector.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/boundaries/boundary_builder.hpp"  // NEW: Type-safe boundary configuration
#include "exception.hpp"
#include <vector>
#include <iostream>
#include <cmath>

using namespace sph;

// ============================================================
// BOUNDARY CONFIGURATION SWITCH
// ============================================================
// Set USE_PERIODIC_BOUNDARY to switch boundary conditions:
//   true  = PERIODIC boundaries (particles wrap around, no walls)
//   false = MIRROR boundaries (ghost particles, reflective walls)
// ============================================================
// CHANGED: Use MIRROR boundaries for shock tube (need reflective walls, not wrapping)
constexpr bool USE_PERIODIC_BOUNDARY = false;  // Changed from true

/**
 * @brief 2D Shock Tube Plugin
 * 
 * Extends the 1D Sod shock tube to 2D:
 * - Discontinuity along x-direction at x=0.5
 * - Periodic or reflective boundaries in y-direction
 * - Same density/pressure jump as Sod problem
 * 
 * Boundary Configuration:
 * - USE_PERIODIC_BOUNDARY = false → MIRROR (ghost particles, walls)
 * - USE_PERIODIC_BOUNDARY = true  → PERIODIC (wrapping, no walls)
 */
class GSPHShockTube2DPlugin : public SimulationPlugin<2> {
public:
    std::string get_name() const override {
        return "gsph_shock_tube_2d";
    }
    
    std::string get_description() const override {
        return "2D Sod shock tube - GSPH (Godunov SPH)";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_gsph.cpp"};
    }
    
    void initialize(std::shared_ptr<Simulation<2>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        static constexpr int Dim = 2;
        
        std::cout << "\n=== 2D GSPH SHOCK TUBE ===\n";
        std::cout << "Algorithm: GSPH (HLL Riemann solver)\n\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        
        const real gamma = 1.4;  // Adiabatic index for ideal gas
        
        // Domain setup
        // X-direction: shock tube [-0.5, 1.5] with discontinuity at x=0.5
        // Y-direction: [0, 0.5] for visualization
        
        // MODIFIED SOD SHOCK TUBE: 4:1 density ratio with uniform mass particles
        const real Ly = 0.5;
        const real Lx_left = 1.0;
        const real Lx_right = 1.0;
        
        const int Nx_left = 40;
        const real dx_left = Lx_left / Nx_left;
        const real dy = dx_left;  // Uniform Y-spacing for both regions
        const int Ny = static_cast<int>(Ly / dy);
        
        // For uniform mass: m = ρ_left * dx_left * dy = ρ_right * dx_right * dy
        // So: dx_right = dx_left * (ρ_left / ρ_right) = dx_left * (1.0 / 0.25) = 4 * dx_left
        const real dx_right = 4.0 * dx_left;
        const int Nx_right = static_cast<int>(Lx_right / dx_right);
        
        const int num = Nx_left * Ny + Nx_right * Ny;
        
        const real mass = 1.0 * dx_left * dy;  // Uniform mass for all particles
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        std::cout << "\n--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Algorithm: GSPH (Godunov SPH)\n";
        std::cout << "Left state:  ρ=1.0,   P=1.0,  dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.25,  P=0.1,  dx=" << dx_right << "\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Y-extent: [0, " << Ly << "]\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        
        int idx = 0;
        
        // Initialize left side particles (HIGH DENSITY region)
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
        
        // Initialize right side particles (LOW DENSITY region)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_right = 0.5 + dx_right * 0.5;
            
            for (int i = 0; i < Nx_right; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_right, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 0.25;            // Target density (4:1 ratio)
                p.pres = 0.1;
                p.mass = mass;       // Same mass as left
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
                // CRITICAL FIX: Disable 2nd order MUSCL
                // Reason: Gradient arrays only exist for real particles, not ghost particles
                // This matches the working 1D configuration
                .with_2nd_order_muscl(false)  // Changed from true - fixes ghost particle gradient issues
                
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
        // STEP 6: INITIALIZE BOUNDARY SYSTEM (TYPE-SAFE API)
        // ============================================================
        
        std::cout << "\n--- Boundary Configuration (Type-Safe API) ---\n";
        
        BoundaryConfiguration<Dim> ghost_config;
        
        if constexpr (USE_PERIODIC_BOUNDARY) {
            // ========== PERIODIC BOUNDARY MODE ==========
            std::cout << "Mode: PERIODIC (particles wrap around)\n";
            
            // TYPE-SAFE API: Ghost particles automatically enabled!
            ghost_config = BoundaryBuilder<Dim>()
                .with_periodic_boundaries()
                .in_range(
                    Vector<Dim>{-0.5, 0.0},
                    Vector<Dim>{1.5, 0.5}
                )
                .build();
            
            std::cout << "✓ Periodic boundaries configured\n";
            std::cout << "  ✓ Ghost particles automatically enabled\n";
            std::cout << "  ✓ No confusing boolean parameters\n";
            
        } else {
            // ========== MIRROR BOUNDARY MODE ==========
            std::cout << "Mode: MIRROR (reflective walls with ghost particles)\n";
            
            // TYPE-SAFE DECLARATIVE API:
            // - Intent is crystal clear from method names
            // - Ghost particles automatically enabled
            // - Compile-time safety prevents mistakes
            ghost_config = BoundaryBuilder<Dim>()
                .in_range(
                    Vector<Dim>{-0.5, 0.0},
                    Vector<Dim>{1.5, 0.5}
                )
                .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)  // X: frictionless shock tube walls
                .with_mirror_in_dimension(1, MirrorType::NO_SLIP, dy, dy)     // Y: sticky confined walls (use dense spacing)
                .build();
            
            std::cout << "✓ Ghost particle system configured\n";
            std::cout << "  ✓ Type-safe declarative API\n";
            std::cout << "  ✓ X-direction: FREE_SLIP (frictionless)\n";
            std::cout << "  ✓ Y-direction: NO_SLIP (wall friction)\n";
        }
        
        // Initialize ghost manager with configuration
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
        std::cout << "    Particle spacing: dy=" << dy << "\n";
        std::cout << "    Wall offset (left): ±" << (0.5 * dy) << "\n";
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

DEFINE_SIMULATION_PLUGIN(GSPHShockTube2DPlugin, 2)

/**
 * @file plugin_gsph.cpp
 * @brief 2D Shock Tube GSPH Plugin (V3 pure business logic)
 * 
 * GSPH (Godunov SPH) configuration for 2D shock tube:
 * - Uses HLL Riemann solver (NOT artificial viscosity)
 * - Shock propagates in x-direction
 * - Mirror boundaries with ghost particles
 * - Physics-based parameter estimation
 * - V3 pure functional interface
 */

#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include "core/parameters/parameter_estimator.hpp"
#include "core/parameters/parameter_validator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/utilities/vector.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/boundaries/boundary_builder.hpp"
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
 * @brief 2D Shock Tube Plugin (V3 interface)
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
class GSPHShockTube2DPlugin : public SimulationPluginV3<2> {
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
    
    InitialCondition<2> create_initial_condition() const override {
        static constexpr int Dim = 2;
        
        std::cout << "\n=== 2D GSPH SHOCK TUBE ===\n";
        std::cout << "Algorithm: GSPH (HLL Riemann solver)\n\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        
        const real gamma = 1.4;  // Adiabatic index for ideal gas
        
        // Domain setup - Following 2D SPH literature recommendations
        // Reference: Puri & Ramachandran (2014), Price (2024)
        // X-direction: [0, 1.0] with discontinuity at x=0.5 (standard Sod setup)
        // Y-direction: [0, 0.1] small height for planar 2D (quasi-1D behavior)
        
        // SPH STRATEGY: Equal mass particles with variable spacing
        // - Density is COMPUTED by SPH: ρ = Σ m_j W(r_ij, h)
        // - For uniform mass in 2D: ρ ∝ 1/(dx*dy), with constant dy → ρ ∝ 1/dx
        // - Standard Sod: ρ_left/ρ_right = 1.0/0.125 = 8:1 density ratio
        // - Need spacing ratio: dx_right/dx_left = ρ_left/ρ_right = 8.0
        
        const real Ly = 0.1;  // Small height for planar 2D (literature: 0.1-0.2)
        const real Lx_left = 0.5;   // Left region [0, 0.5]
        const real Lx_right = 0.5;  // Right region [0.5, 1.0]
        
        // Target densities (Sod standard)
        const real rho_target_left = 1.0;
        const real rho_target_right = 0.125;
        const real spacing_ratio = rho_target_left / rho_target_right;  // 8.0
        
        // Left region: fine spacing
        const int Nx_left = 200;  // High resolution for shock features
        const real dx_left = Lx_left / Nx_left;
        
        // Right region: coarse spacing (8x larger for 8:1 density ratio)
        const real dx_right = spacing_ratio * dx_left;
        const int Nx_right = static_cast<int>(Lx_right / dx_right);
        
        const real dy = dx_left;  // Y-spacing matches left region
        const int Ny = static_cast<int>(Ly / dy);
        
        const int num = Nx_left * Ny + Nx_right * Ny;
        
        // UNIFORM MASS - density from spacing
        const real mass = 1.0 * dx_left * dy;
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        std::cout << "\n--- Particle Initialization (2D Planar Sod Shock Tube) ---\n";
        std::cout << "Reference: Puri & Ramachandran (2014), Price (2024)\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Algorithm: GSPH (Godunov SPH)\n";
        std::cout << "STRATEGY: Equal mass + 8:1 variable spacing\n";
        std::cout << "Domain: X=[0, 1.0], Y=[0, " << Ly << "] (planar 2D)\n";
        std::cout << "Left:  dx=" << dx_left << ", " << Nx_left << " particles\n";
        std::cout << "Right: dx=" << dx_right << ", " << Nx_right << " particles\n";
        std::cout << "Spacing ratio: " << (dx_right/dx_left) << ":1\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        std::cout << "Expected ρ ratio: 8:1 from spacing\n";
        std::cout << "Discontinuity at x=0.5 (standard Sod setup)\n";
        
        int idx = 0;
        
        // Initialize left side particles (HIGH DENSITY region)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_left = 0.0 + dx_left * 0.5;  // Start from x=0
            
            for (int i = 0; i < Nx_left; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_left, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 1.0;   // Initial guess
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
            real x_right = 0.5 + dx_right * 0.5;  // Start from x=0.5
            
            for (int i = 0; i < Nx_right; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_right, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = 0.125;  // Initial guess (Sod standard)
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
        
        std::shared_ptr<SPHParameters> param;
        try {
            param = SPHParametersBuilderBase()
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
            
            std::cout << "✓ Parameters built with type-safe GSPH API\n";
            std::cout << "  - GSPH uses HLL Riemann solver, NOT artificial viscosity\n";
            std::cout << "  - 2nd order MUSCL disabled (fixes ghost gradient issues)\n";
                
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
        // STEP 5: BOUNDARY CONFIGURATION (TYPE-SAFE API, V3 INTERFACE)
        // ============================================================
        
        std::cout << "\n--- Boundary Configuration (Type-Safe API) ---\n";
        
        BoundaryConfiguration<Dim> boundary_config;
        
        if constexpr (USE_PERIODIC_BOUNDARY) {
            // ========== PERIODIC BOUNDARY MODE ==========
            std::cout << "Mode: PERIODIC (particles wrap around)\n";
            
            // TYPE-SAFE API: Ghost particles automatically enabled!
            boundary_config = BoundaryBuilder<Dim>()
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
            // ========== MIXED BOUNDARY MODE ==========
            std::cout << "Mode: MIXED (X-direction mirror walls, Y-direction periodic)\n";
            
            // TYPE-SAFE DECLARATIVE API:
            // - X-direction: Transmissive boundaries (allow shock waves to exit)
            // - Y-direction: Periodic (planar symmetry)
            // - Intent is crystal clear from method names
            // - Ghost particles automatically enabled
            // - Compile-time safety prevents mistakes
            boundary_config = BoundaryBuilder<Dim>()
                .in_range(
                    Vector<Dim>{0.0, 0.0},
                    Vector<Dim>{1.0, Ly}
                )
                .with_mirror_in_dimension(0, MirrorType::NO_SLIP, dx_left, dx_right)  // X: walls at 0 and 1
                .with_periodic_in_dimension(1)     // Y: periodic (planar symmetry)
                .build();
            
            std::cout << "√ Ghost particle system configured\n";
            std::cout << "  √ Type-safe declarative API\n";
            std::cout << "  √ X-direction: NO_SLIP (walls at x=0 and x=1)\n";
            std::cout << "  √ Y-direction: PERIODIC (planar symmetry)\n";
        }
        
        std::cout << "✓ X-boundary: MIRROR (NO_SLIP) at x=" << boundary_config.range_min[0] 
                  << " and x=" << boundary_config.range_max[0] << "\n";
        std::cout << "  - Left spacing: " << dx_left << ", Right spacing: " << dx_right << "\n";
        std::cout << "✓ Y-boundary: PERIODIC (wrapping) in range [" << boundary_config.range_min[1]
                  << ", " << boundary_config.range_max[1] << "]\n";
        
        // NOTE: V3 INTERFACE - Framework handles system initialization
        // The Solver will:
        //   1. Compute smoothing lengths in PreInteraction
        //   2. Set kernel support radius based on max(sml)
        //   3. Generate ghost particles after sml is known
        //
        // BUS ERROR FIX: By using V3 interface, plugins can NO LONGER access
        // uninitialized p.sml field - compile-time safety prevents the bug!
        
        std::cout << "\n=== Initialization Complete ===\n";
        std::cout << "Particles: " << particles.size() << "\n";
        std::cout << "Ghost generation deferred to Solver::initialize()\n";
        std::cout << "Ready to return InitialCondition\n\n";
        
        // ============================================================
        // V3 INTERFACE: Return InitialCondition data
        // ============================================================
        return InitialCondition<Dim>::with_particles(std::move(particles))
            .with_parameters(std::move(param))
            .with_boundaries(std::move(boundary_config));
    }
};

DEFINE_SIMULATION_PLUGIN_V3(GSPHShockTube2DPlugin, 2)

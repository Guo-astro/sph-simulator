/**
 * @file plugin_ssph.cpp
 * @brief 2D Shock Tube SSPH Plugin (V3 pure business logic)
 * 
 * SSPH configuration for 2D shock tube:
 * - SSPH (artificial viscosity)
 * - Periodic boundaries
 * - Physics-based parameter estimation
 * - V3 pure functional interface (compile-time safety against uninitialized state)
 */

#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
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

class SSPHShockTube2DPlugin : public SimulationPluginV3<2> {
public:
    std::string get_name() const override {
        return "ssph_shock_tube_2d";
    }
    
    std::string get_description() const override {
        return "2D Sod shock tube - SSPH (no ghosts)";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_ssph.cpp"};
    }
    
    InitialCondition<2> create_initial_condition() const override {
        static constexpr int Dim = 2;
        
        std::cout << "\n=== 2D SSPH SHOCK TUBE ===\n";
        std::cout << "Mode: MIRROR boundaries (matching GSPH configuration)\n\n";
        
        const real gamma = 1.4;
        
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
        
        std::cout << "--- Particle Initialization (2D Planar Sod Shock Tube) ---\n";
        std::cout << "Reference: Puri & Ramachandran (2014), Price (2024)\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Algorithm: SSPH (artificial viscosity)\n";
        std::cout << "STRATEGY: Equal mass + 8:1 variable spacing\n";
        std::cout << "Domain: X=[0, 1.0], Y=[0, " << Ly << "] (planar 2D)\n";
        std::cout << "Left:  dx=" << dx_left << ", " << Nx_left << " particles\n";
        std::cout << "Right: dx=" << dx_right << ", " << Nx_right << " particles\n";
        std::cout << "Spacing ratio: " << (dx_right/dx_left) << ":1\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        std::cout << "Expected ρ ratio: 8:1 from spacing\n";
        std::cout << "Discontinuity at x=0.5 (standard Sod setup)\n";
        
        int idx = 0;
        
        // Initialize left side particles (HIGH density region)
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
        
        // Initialize right side particles (LOW density region)
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
        
        // Estimate physics-based parameters
        std::cout << "--- Parameter Estimation ---\n";
        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(particles, 2.0);
        
        std::cout << "Suggested parameters:\n";
        std::cout << "  CFL sound: " << suggestions.cfl_sound << "\n";
        std::cout << "  CFL force: " << suggestions.cfl_force << "\n";
        std::cout << "  Neighbor number: " << suggestions.neighbor_number << "\n\n";
        
        // Build SSPH parameters
        std::cout << "--- Building SSPH Parameters ---\n";
        
        std::shared_ptr<SPHParameters> param;
        try {
            param = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_physics(suggestions.neighbor_number, gamma)
                .with_cfl(suggestions.cfl_sound, suggestions.cfl_force)
                .with_kernel("cubic_spline")
                .with_iterative_smoothing_length(true)  // Re-enabled with mirror boundaries
                
                // Transition to SSPH
                .as_ssph()
                .with_artificial_viscosity(1.0)  // alpha = 1.0
                .build();
            
            std::cout << "✓ SSPH parameters set\n";
            std::cout << "  neighbor_number = " << suggestions.neighbor_number << "\n";
            std::cout << "  artificial_viscosity = 1.0\n";
            std::cout << "  boundaries = mirror (reflective walls)\n";
                
        } catch (const std::exception& e) {
            THROW_ERROR("Parameter building failed: " + std::string(e.what()));
        }
        
        // Validation
        std::cout << "\n--- Parameter Validation ---\n";
        try {
            ParameterValidator::validate_all<Dim>(particles, param);
            std::cout << "✓ SSPH parameters validated\n";
        } catch (const std::exception& e) {
            std::cout << "⚠ Validation warning: " << e.what() << "\n";
        }
        
        // Boundary configuration (mixed boundaries)
        std::cout << "\n--- Boundary Configuration ---\n";
        std::cout << "Mode: MIXED (X-direction mirror walls, Y-direction periodic)\n";
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .in_range(
                Vector<Dim>{0.0, 0.0},
                Vector<Dim>{1.0, Ly}
            )
            .with_mirror_in_dimension(0, MirrorType::NO_SLIP, dx_left, dx_right)
            .with_periodic_in_dimension(1)
            .build();
        
        std::cout << "✓ Mixed boundaries configured\n";
        std::cout << "  X-direction: NO_SLIP (walls at x=0 and x=1)\n";
        std::cout << "  Y-direction: PERIODIC (planar symmetry)\n";
        
        // NOTE: V3 INTERFACE - Framework handles system initialization
        // The Solver will:
        //   1. Compute smoothing lengths in PreInteraction
        //   2. Set kernel support radius based on max(sml)
        //   3. Generate ghost particles after sml is known
        //
        // BUS ERROR FIX: By using V3 interface, plugins can NO LONGER access
        // uninitialized p.sml field - compile-time safety prevents the bug!
        
        std::cout << "  Ghost generation deferred to Solver::initialize()\n";
        std::cout << "\n=== 2D SSPH Initialization Complete ===\n\n";
        
        // ============================================================
        // V3 INTERFACE: Return InitialCondition data
        // ============================================================
        return InitialCondition<Dim>::with_particles(std::move(particles))
            .with_parameters(std::move(param))
            .with_boundaries(std::move(boundary_config));
    }
};

DEFINE_SIMULATION_PLUGIN_V3(SSPHShockTube2DPlugin, 2)

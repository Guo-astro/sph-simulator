/**
 * @file plugin_3d.cpp
 * @brief 3D Sod shock tube plugin with physics-based parameters (V3 pure business logic)
 * 
 * This plugin creates a 3D shock tube simulation with:
 * - X-direction: Shock tube with density discontinuity at x=0.5
 * - Y-direction: Uniform cross-section
 * - Z-direction: Uniform cross-section
 * 
 * Features:
 * - Per-boundary particle spacing for accurate wall positioning
 * - Physics-based parameter estimation
 * - Morris 1997 ghost particle boundaries
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

class ShockTubePlugin3D : public SimulationPluginV3<3> {
public:
    std::string get_name() const override {
        return "shock_tube_3d";
    }
    
    std::string get_description() const override {
        return "3D Sod shock tube with per-boundary spacing";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }

    std::vector<std::string> get_source_files() const override {
        return {"plugin_3d.cpp"};
    }

    InitialCondition<3> create_initial_condition() const override {
        static constexpr int Dim = 3;
        
        std::cout << "\n=== 3D SHOCK TUBE SIMULATION ===\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        
        const real gamma = 1.4;
        
        // Domain setup
        // X-direction: shock tube [-0.5, 1.5] with discontinuity at x=0.5
        // Y-direction: [0, 0.5] uniform
        // Z-direction: [0, 0.5] uniform
        
        // Right side (lower density)
        // REDUCED RESOLUTION for faster computation (was 30×15×15 → 101,250 particles)
        const int Nx_right = 15;  // Reduced from 30
        const int Ny = 8;         // Reduced from 15
        const int Nz = 8;         // Reduced from 15 → ~13,800 particles
        const real Lx_right = 1.0;  // [0.5, 1.5]
        const real Ly = 0.5;
        const real Lz = 0.5;
        const real dx_right = Lx_right / Nx_right;
        const real dy = Ly / Ny;
        const real dz = Lz / Nz;
        
        // Left side (higher density)
        const real Lx_left = 1.0;  // [-0.5, 0.5]
        const real dx_left = dx_right / 8.0;  // 8× denser
        const int Nx_left = static_cast<int>(Lx_left / dx_left);
        
        const int num = (Nx_left + Nx_right) * Ny * Nz;
        const real mass = 0.125 * dx_right * dy * dz;
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        std::cout << "\n--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << " × " << Nz << "\n";
        std::cout << "Left state:  ρ=1.0,   P=1.0,  dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.125, P=0.1,  dx=" << dx_right << "\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Y-extent: [0, " << Ly << "], Z-extent: [0, " << Lz << "]\n";
        
        int idx = 0;
        
        // Initialize left side particles
        for (int k = 0; k < Nz; ++k) {
            real z = dz * (k + 0.5);
            for (int j = 0; j < Ny; ++j) {
                real y = dy * (j + 0.5);
                real x_left = -0.5 + dx_left * 0.5;
                
                for (int i = 0; i < Nx_left; ++i) {
                    auto& p = particles[idx];
                    p.pos = Vector<Dim>{x_left, y, z};
                    p.vel = Vector<Dim>{0.0, 0.0, 0.0};
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
        }
        
        // Initialize right side particles
        for (int k = 0; k < Nz; ++k) {
            real z = dz * (k + 0.5);
            for (int j = 0; j < Ny; ++j) {
                real y = dy * (j + 0.5);
                real x_right = 0.5 + dx_right * 0.5;
                
                for (int i = 0; i < Nx_right; ++i) {
                    auto& p = particles[idx];
                    p.pos = Vector<Dim>{x_right, y, z};
                    p.vel = Vector<Dim>{0.0, 0.0, 0.0};
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
        }
        
        // ============================================================
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================
        
        std::cout << "\n--- Parameter Estimation ---\n";
        
        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(particles, 2.0);
        
        std::cout << "\nSuggested parameters:\n";
        std::cout << "  CFL sound: " << suggestions.cfl_sound << "\n";
        std::cout << "  CFL force: " << suggestions.cfl_force << "\n";
        std::cout << "  Neighbor number: " << suggestions.neighbor_number << "\n";
        
        // ============================================================
        // STEP 3: BUILD PARAMETERS (TYPE-SAFE API)
        // ============================================================
        
        std::cout << "\n--- Building Parameters (Type-Safe API) ---\n";
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 0.2, 0.01)
            .with_physics(suggestions.neighbor_number, gamma)
            .with_cfl(suggestions.cfl_sound, suggestions.cfl_force)
            .with_kernel("cubic_spline")
            .with_iterative_smoothing_length(true)
            
            // Transition to GSPH (uses Riemann solver, not artificial viscosity)
            .as_gsph()
            .with_2nd_order_muscl(false)  // Disable 2nd order for ghost compatibility
            
            .build();
        
        std::cout << "✓ Parameters built with type-safe GSPH API\n";
        std::cout << "  - GSPH uses HLL Riemann solver\n";
        std::cout << "  - 1st order for stability with ghost particles\n";
        
        // ============================================================
        // STEP 4: VALIDATE PARAMETERS
        // ============================================================
        
        std::cout << "\n--- Parameter Validation ---\n";
        
        try {
            ParameterValidator::validate_all<Dim>(particles, params);
            std::cout << "✓ All parameters validated successfully\n";
        } catch (const std::exception& e) {
            std::cout << "⚠ Validation warning: " << e.what() << "\n";
        }
        
        // ============================================================
        // STEP 5: BOUNDARY CONFIGURATION (TYPE-SAFE API, V3 INTERFACE)
        // ============================================================
        std::cout << "\n--- Ghost Particle System ---\n";
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .in_range(
                Vector<Dim>{-0.5, 0.0, 0.0},
                Vector<Dim>{1.5, 0.5, 0.5}
            )
            .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)  // X: frictionless
            .with_mirror_in_dimension(1, MirrorType::NO_SLIP, dy, dy)               // Y: sticky walls
            .with_mirror_in_dimension(2, MirrorType::NO_SLIP, dz, dz)               // Z: sticky walls
            .build();
        
        std::cout << "✓ Ghost particle system configured\n";
        std::cout << "  X-boundary: MIRROR (FREE_SLIP) [" << boundary_config.range_min[0] 
                  << ", " << boundary_config.range_max[0] << "]\n";
        std::cout << "    Left spacing:  " << dx_left << " → wall at " 
                  << boundary_config.get_wall_position(0, false) << "\n";
        std::cout << "    Right spacing: " << dx_right << " → wall at " 
                  << boundary_config.get_wall_position(0, true) << "\n";
        std::cout << "  Y-boundary: MIRROR (NO_SLIP) [" << boundary_config.range_min[1]
                  << ", " << boundary_config.range_max[1] << "]\n";
        std::cout << "    Spacing: " << dy << "\n";
        std::cout << "  Z-boundary: MIRROR (NO_SLIP) [" << boundary_config.range_min[2]
                  << ", " << boundary_config.range_max[2] << "]\n";
        std::cout << "    Spacing: " << dz << "\n";
        
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
            .with_parameters(std::move(params))
            .with_boundaries(std::move(boundary_config));
    }
};

DEFINE_SIMULATION_PLUGIN_V3(ShockTubePlugin3D, 3)

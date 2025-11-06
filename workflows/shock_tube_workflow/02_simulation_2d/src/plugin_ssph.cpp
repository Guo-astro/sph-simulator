/**
 * @file plugin_ssph.cpp
 * @brief 2D Shock Tube SSPH Plugin (Standard SPH with no ghost particles)
 * 
 * SSPH configuration for 2D shock tube:
 * - SSPH (artificial viscosity)
 * - neighbor_number = 4
 * - No ghost particles (baseline verification mode)
 * - Periodic boundaries (legacy mode)
 */

#include "core/simulation_plugin.hpp"
#include "core/sph_parameters_builder_base.hpp"
#include "core/ssph_parameters_builder.hpp"
#include "core/parameter_estimator.hpp"
#include "core/parameter_validator.hpp"
#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/vector.hpp"
#include "core/ghost_particle_manager.hpp"
#include "core/boundary_types.hpp"
#include "core/boundary_builder.hpp"
#include "exception.hpp"
#include <vector>
#include <iostream>
#include <cmath>

using namespace sph;

class SSPHShockTube2DPlugin : public SimulationPlugin<2> {
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
    
    void initialize(std::shared_ptr<Simulation<2>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        static constexpr int Dim = 2;
        
        std::cout << "\n=== 2D SSPH SHOCK TUBE ===\n";
        std::cout << "Mode: NO GHOSTS\n\n";
        
        const real gamma = 1.4;
        
        // OPTIMIZED: Uniform mass particles, vary X-spacing for density ratio
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
        
        std::cout << "--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Algorithm: SSPH (artificial viscosity)\n";
        std::cout << "Left state:  ρ=1.0,  P=1.0,  dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.25, P=0.1,  dx=" << dx_right << "\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Y-extent: [0, " << Ly << "]\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        
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
                p.dens = 0.25;
                p.pres = 0.1;
                p.mass = mass;
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.sound = std::sqrt(gamma * p.pres / p.dens);
                p.id = idx;
                
                x_right += dx_right;
                ++idx;
            }
        }
        
        // Build SSPH parameters
        std::cout << "--- Building SSPH Parameters ---\n";
        
        // Estimate physics-based parameters (but use conservative CFL for stability)
        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(particles, 2.0);
        
        std::cout << "Suggested neighbor_number: " << suggestions.neighbor_number << "\n";
        std::cout << "Using conservative CFL values for stability\n\n";
        
        try {
            auto built_params = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_physics(suggestions.neighbor_number, gamma)  // Use estimated neighbor number for 2D
                .with_cfl(0.3, 0.25)  // Conservative baseline CFL values
                .with_kernel("cubic_spline")
                .with_iterative_smoothing_length(true)
                .with_periodic_boundary(
                    std::array<real, 3>{-0.5, 0.0, 0.0},
                    std::array<real, 3>{1.5, 0.5, 0.0}
                )
                
                // Transition to SSPH
                .as_ssph()
                .with_artificial_viscosity(1.0)  // alpha = 1.0
                .build();
            
            *param = *built_params;
            
            std::cout << "✓ SSPH parameters set\n";
            std::cout << "  neighbor_number = " << suggestions.neighbor_number << "\n";
            std::cout << "  artificial_viscosity = 1.0\n";
            std::cout << "  periodic = true (legacy mode)\n";
                
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
        
        sim->particles = std::move(particles);
        sim->particle_num = num;
        
        // Boundary configuration (periodic, ghost particles auto-enabled)
        std::cout << "\n--- Boundary Configuration ---\n";
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(
                Vector<Dim>{-0.5, 0.0},
                Vector<Dim>{1.5, 0.5}
            )
            .build();
        
        sim->ghost_manager->initialize(boundary_config);
        
        real max_sml = 0.0;
        for (const auto& p : sim->particles) {
            max_sml = std::max(max_sml, p.sml);
        }
        sim->ghost_manager->set_kernel_support_radius(max_sml * 2.0);
        sim->ghost_manager->generate_ghosts(sim->particles);
        
        std::cout << "✓ Periodic boundaries configured\n";
        std::cout << "  Ghost particles: " << sim->ghost_manager->get_ghost_count() << "\n";
        std::cout << "\n=== 2D SSPH Initialization Complete ===\n\n";
    }
};

DEFINE_SIMULATION_PLUGIN(SSPHShockTube2DPlugin, 2)

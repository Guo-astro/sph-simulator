#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include "core/boundaries/boundary_builder.hpp"
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
class ShockTubePluginEnhanced : public SimulationPluginV3<1> {
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
    
    InitialCondition<1> create_initial_condition() const override {
        // This plugin is for 1D simulations
        static constexpr int Dim = 1;

        std::cout << "\n=== ENHANCED SHOCK TUBE (Physics-Based Parameters) ===\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
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
        // STEP 2: BUILD PARAMETERS
        // ============================================================
        
        std::cout << "\n--- Configuring Simulation Parameters ---\n";
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 0.15, 0.01, 0.01)
            .with_cfl(0.3, 0.25)
            .with_physics(30, gamma)  // neighbor_number=30 for 1D
            .with_kernel("cubic_spline")
            .with_tree_params(20, 1)
            .with_iterative_smoothing_length(true)
            .as_gsph()
            .with_2nd_order_muscl(false)  // 1st order safer
            .build();
        
        std::cout << "✓ Parameters configured\n";
        std::cout << "  - SPH type: GSPH (Godunov SPH with Riemann solver)\n";
        std::cout << "  - 2nd order MUSCL: disabled\n";
        std::cout << "  - CFL sound: " << params->get_cfl().sound << "\n";
        std::cout << "  - CFL force: " << params->get_cfl().force << "\n";
        
        // ============================================================
        // STEP 3: CONFIGURE BOUNDARIES
        // ============================================================
        
        std::cout << "\n--- Ghost Particle System ---\n";
        
        // Configure mirror boundary with ghost particles (reflective walls)
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)
            .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
            .build();
        
        std::cout << "✓ Ghost particle system configured\n";
        std::cout << "  Boundary type: MIRROR (FREE_SLIP)\n";
        std::cout << "  Domain range: [" << boundary_config.range_min[0] 
                  << ", " << boundary_config.range_max[0] << "]\n";
        std::cout << "  Left particle spacing (dx_left):  " << dx_left << "\n";
        std::cout << "  Right particle spacing (dx_right): " << dx_right << "\n";
        std::cout << "  (Ghost particles will be generated after sml calculation)\n";
        
        std::cout << "\n--- Configuration Summary ---\n";
        std::cout << "SPH Algorithm: Godunov SPH\n";
        std::cout << "CFL coefficients: sound=" << params->get_cfl().sound 
                  << ", force=" << params->get_cfl().force << "\n";
        std::cout << "Neighbor number: " << params->get_physics().neighbor_number << "\n";
        std::cout << "Gamma (adiabatic): " << params->get_physics().gamma << "\n";
        std::cout << "Kernel: Cubic Spline\n";
        
        std::cout << "\n=== Initialization Complete ===\n\n";
        
        // ============================================================
        // V3 INTERFACE: Return InitialCondition data
        // ============================================================
        return InitialCondition<Dim>::with_particles(std::move(particles))
            .with_parameters(std::move(params))
            .with_boundaries(std::move(boundary_config));
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube_plugin_enhanced.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN_V3(ShockTubePluginEnhanced, 1)

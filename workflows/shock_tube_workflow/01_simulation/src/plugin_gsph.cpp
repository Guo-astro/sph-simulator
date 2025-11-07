#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include "core/parameters/parameter_estimator.hpp"
#include "core/parameters/parameter_validator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/boundaries/boundary_builder.hpp"  // NEW: Type-safe boundary configuration
#include "exception.hpp"
#include <vector>
#include <iostream>

using namespace sph;

/**
 * @brief GSPH (Godunov SPH) shock tube plugin using HLL Riemann solver.
 * 
 * This plugin implements Godunov SPH for shock capturing:
 * - Uses HLL Riemann solver (NOT artificial viscosity)
 * - 1st order scheme (2nd order disabled due to ghost particle gradient issues)
 * - Physics-based CFL calculation from von Neumann stability analysis
 * - Parameter validation against actual particle configuration
 * 
 * CFL Theory:
 * - dt_sound = CFL_sound * h / (c_s + |v|)  [Monaghan 2005]
 * - dt_force = CFL_force * sqrt(h / |a|)    [Monaghan 1989]
 * 
 * Reference: Inutsuka 2002 - GSPH with Riemann Solver
 */
class ShockTubeGSPHPlugin : public SimulationPluginV3<1> {
public:
    std::string get_name() const override {
        return "shock_tube_gsph";
    }
    
    std::string get_description() const override {
        return "1D Sod shock tube with GSPH (Godunov SPH, HLL Riemann solver)";
    }
    
    std::string get_version() const override {
        return "1.0.0";  // GSPH implementation
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_gsph.cpp"};
    }
    
    InitialCondition<1> create_initial_condition() const override {
        static constexpr int Dim = 1;

        std::cout << "\n=== GSPH SHOCK TUBE (Godunov SPH with HLL Riemann Solver) ===\n";
        
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
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================
        
        std::cout << "\n--- Physics-Based Parameter Estimation ---\n";
        
        auto suggestions = ParameterEstimator::suggest_parameters(particles);
        
        std::cout << "\nEstimated parameters from particle analysis:\n";
        std::cout << "  CFL_sound = " << suggestions.cfl_sound 
                  << " (from dt = CFL * h / (c_s + |v|))\n";
        std::cout << "  CFL_force = " << suggestions.cfl_force
                  << " (from dt = CFL * sqrt(h / |a|))\n";
        std::cout << "  neighbor_number = " << suggestions.neighbor_number
                  << " (from kernel support volume)\n";
        
        std::cout << "\nRationale:\n";
        std::cout << suggestions.rationale << "\n";
        
        // ============================================================
        // STEP 3: BUILD PARAMETERS WITH ESTIMATED VALUES
        // ============================================================
        
        std::cout << "\n--- Building Parameter Set (Type-Safe GSPH API) ---\n";
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 0.30, 0.01, 0.01)
            .with_cfl(suggestions.cfl_sound, suggestions.cfl_force)
            .with_physics(suggestions.neighbor_number, gamma)
            .with_kernel("cubic_spline")
            .with_tree_params(20, 1)
            .with_iterative_smoothing_length(true)
            .as_gsph()
            .with_2nd_order_muscl(false)  // Disable 2nd order with ghosts
            .build();
        
        std::cout << "✓ Parameters built with type-safe GSPH API\n";
        std::cout << "  - GSPH uses Riemann solver (HLL), NOT artificial viscosity\n";
        std::cout << "  - 2nd order MUSCL disabled (1st order safer with ghosts)\n";
        
        // ============================================================
        // STEP 4: VALIDATE PARAMETERS AGAINST PARTICLES
        // ============================================================
        
        std::cout << "\n--- Parameter Validation ---\n";
        
        try {
            ParameterValidator::validate_all(particles, params);
            std::cout << "✓ All parameters validated - SAFE to run!\n";
            
            // Show what timestep we'll get (using getter methods)
            auto config = ParameterEstimator::analyze_particle_config(particles);
            real dt_sound = params->get_cfl().sound * config.avg_spacing / config.max_sound_speed;
            real dt_force = std::numeric_limits<real>::infinity();
            if (config.max_acceleration > 1e-10) {
                dt_force = params->get_cfl().force * std::sqrt(config.avg_spacing / config.max_acceleration);
            }
            
            std::cout << "\nExpected timestep:\n";
            std::cout << "  dt_sound = " << dt_sound << "\n";
            std::cout << "  dt_force = " << (dt_force < 1e100 ? std::to_string(dt_force) : "inf") << "\n";
            std::cout << "  dt_actual = " << std::min(dt_sound, dt_force) << "\n";
            
        } catch (const std::runtime_error& e) {
            std::cerr << "\n✖ VALIDATION FAILED!\n";
            std::cerr << e.what() << "\n";
            std::cerr << "\nSimulation will NOT run - parameters are unsafe!\n";
            THROW_ERROR("Parameter validation failed");
        }
        
        // ============================================================
        // STEP 5: CONFIGURE BOUNDARIES
        // ============================================================
        
        std::cout << "\n--- Ghost Particle System (Type-Safe API) ---\n";
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_mirror_in_dimension(0, MirrorType::FREE_SLIP, dx_left, dx_right)
            .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
            .build();
        
        std::cout << "✓ Ghost particle system configured (type-safe)\n";
        std::cout << "  ✓ MIRROR boundaries with FREE_SLIP\n";
        std::cout << "  ✓ Asymmetric spacing: left=" << dx_left << ", right=" << dx_right << "\n";
        std::cout << "  ✓ Ghost particles automatically enabled\n";
        
        std::cout << "\n--- Configuration Summary ---\n";
        std::cout << "SPH Algorithm: GSPH (Godunov SPH)\n";
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
};

// V3 Plugin factory
DEFINE_SIMULATION_PLUGIN_V3(ShockTubeGSPHPlugin, 1)

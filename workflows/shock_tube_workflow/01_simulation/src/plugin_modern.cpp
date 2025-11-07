#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include "core/parameters/parameter_estimator.hpp"
#include "core/parameters/parameter_validator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/boundary_builder.hpp"  // NEW: Type-safe API
#include "exception.hpp"
#include <vector>
#include <iostream>

using namespace sph;

/**
 * @brief Modern shock tube plugin with ghost particles
 * 
 * Uses same parameters as baseline abd7353 but enables ghost particles:
 * - neighbor_number = 4
 * - N = 50 (right side)
 * - gamma = 1.4
 * - Domain [-0.5, 1.5]
 * - iterativeSmoothingLength = true
 * 
 * Ghost particles are ENABLED with proper filtering:
 * - Periodic ghosts for boundary support
 * - Ghost filtering in Newton-Raphson (current fix)
 * - Smoothing length determined from real particles only
 * - Forces use both real and ghost particles
 * 
 * This demonstrates modern ghost particle system working correctly
 * with the Newton-Raphson fix applied.
 */
class ModernShockTubePlugin : public SimulationPluginV3<1> {
public:
    std::string get_name() const override {
        return "modern_shock_tube";
    }
    
    std::string get_description() const override {
        return "1D Sod shock tube with ghost particles (modern mode)";
    }
    
    std::string get_version() const override {
        return "modern_with_ghosts";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin_modern.cpp"};
    }
    
    InitialCondition<1> create_initial_condition() const override {
        static constexpr int Dim = 1;

        std::cout << "\n=== MODERN SHOCK TUBE (Ghost Particles Enabled) ===\n";
        std::cout << "Mode: WITH GHOSTS (modern mode with filtering fix)\n\n";
        
        // ============================================================
        // SAME PARAMETERS AS BASELINE
        // ============================================================
        const real gamma = 1.4;
        
        // Right side setup
        const int N_right = 50;
        const real L_right = 1.0;
        const real dx_right = L_right / N_right;
        
        // Left side setup (8× density ratio)
        const real L_left = 1.0;
        const real dx_left = dx_right / 8.0;
        const int N_left = static_cast<int>(L_left / dx_left);
        
        const int num = N_left + N_right;
        const real mass = 0.125 * dx_right;
        
        std::cout << "--- Particle Configuration ---\n";
        std::cout << "Total particles: " << num << " (" << N_left << " left + " << N_right << " right)\n";
        std::cout << "Right particles: N=" << N_right << ", dx=" << dx_right << "\n";
        std::cout << "Left particles:  N=" << N_left << ", dx=" << dx_left << "\n";
        std::cout << "Density ratio: 8:1 (ρ_L/ρ_R = 1.0/0.125)\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        std::cout << "Domain: [-0.5, 1.5]\n";
        std::cout << "Discontinuity at x=0.5\n\n";
        
        // ============================================================
        // PARTICLE INITIALIZATION
        // ============================================================
        std::vector<SPHParticle<Dim>> particles(num);
        
        constexpr real kappa = 1.2;
        const real sml_left = kappa * dx_left;
        const real sml_right = kappa * dx_right;
        
        int idx = 0;
        
        // Left side (high density, high pressure)
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
        
        // Right side (low density, low pressure)
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
        // BUILD PARAMETERS (SAME AS BASELINE)
        // ============================================================
        
        std::cout << "--- Building Parameters (Modern Mode) ---\n";
        
        auto param = SPHParametersBuilderBase()
            .with_time(0.0, 0.30, 0.01, 0.01)
            
            // SAME CFL as baseline
            .with_cfl(0.3, 0.25)
            
            // SAME neighbor_number as baseline
            .with_physics(
                4,      // neighbor_number = 4 (baseline value)
                gamma   // gamma = 1.4
            )
            
            .with_kernel("cubic_spline")
            .with_tree_params(20, 1)
            
            // SAME iterative smoothing as baseline
            .with_iterative_smoothing_length(true)
            
            // Legacy periodic for compatibility
            .with_periodic_boundary(
                std::array<real, 3>{-0.5, 0.0, 0.0},
                std::array<real, 3>{1.5, 0.0, 0.0}
            )
            
            // Transition to SSPH builder and build
            .as_ssph()
            
            // SSPH-specific: Artificial viscosity (required for SSPH)
            .with_artificial_viscosity(
                1.0  // alpha = 1.0 (same as baseline)
            )
            
            .build();
        
        std::cout << "✓ Parameters set (matching baseline values):\n";
        std::cout << "  neighbor_number = " << param->get_physics().neighbor_number << "\n";
        std::cout << "  gamma = " << param->get_physics().gamma << "\n";
        std::cout << "  CFL sound = " << param->get_cfl().sound << "\n";
        std::cout << "  CFL force = " << param->get_cfl().force << "\n";
        std::cout << "  iterative_sml = " << (param->get_iterative_sml() ? "true" : "false") << "\n";
        
        // ============================================================
        // VALIDATION
        // ============================================================
        
        std::cout << "\n--- Parameter Validation ---\n";
        
        try {
            ParameterValidator::validate_all<Dim>(particles, param);
            std::cout << "✓ Parameters validated\n";
        } catch (const std::runtime_error& e) {
            std::cerr << "❌ VALIDATION FAILED: " << e.what() << "\n";
            THROW_ERROR("Parameter validation failed");
        }
        
        // ============================================================
        // MODERN BOUNDARY CONFIGURATION WITH GHOSTS
        // ============================================================
        
        std::cout << "\n--- Boundary Configuration (Type-Safe API) ---\n";
        
        // TYPE-SAFE DECLARATIVE API - Ghost particles automatically enabled!
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(Vector<Dim>{-0.5}, Vector<Dim>{1.5})
            .build();
        
        std::cout << BoundaryBuilder<Dim>::describe(boundary_config);
        std::cout << "\nModern mode with type-safe configuration:\n";
        std::cout << "  ✓ Ghost particles automatically enabled\n";
        std::cout << "  ✓ No confusing boolean parameters\n";
        std::cout << "  ✓ Compile-time safety guarantees\n";
        std::cout << "  ✓ Self-documenting declarative API\n";
        
        std::cout << "\nHow the fix works:\n";
        std::cout << "  1. Neighbor search finds both real and ghost particles\n";
        std::cout << "  2. Ghost particles filtered BEFORE Newton-Raphson iteration\n";
        std::cout << "  3. Smoothing length converges using real neighbors only\n";
        std::cout << "  4. After sml converged, ghosts regenerated with new sml\n";
        std::cout << "  5. Force calculation uses full neighbor list (real + ghost)\n";
        
        std::cout << "\n--- Configuration Summary ---\n";
        std::cout << "SPH Algorithm: SSPH (Standard SPH)\n";
        std::cout << "Artificial Viscosity: α=" << param->get_av().alpha << "\n";
        std::cout << "Kernel: Cubic Spline\n";
        std::cout << "Boundary: Periodic WITH ghosts\n";
        std::cout << "Fix: Ghost filtering in Newton-Raphson\n";
        std::cout << "\n=== Modern Initialization Complete ===\n";
        std::cout << "Ready to run with ghost particles + filtering fix\n\n";
        
        // ============================================================
        // V3 INTERFACE: Return InitialCondition data
        // ============================================================
        return InitialCondition<1>::with_particles(std::move(particles))
            .with_parameters(std::move(param))
            .with_boundaries(std::move(boundary_config));
    }
};

// Plugin factory
DEFINE_SIMULATION_PLUGIN_V3(ModernShockTubePlugin, 1)

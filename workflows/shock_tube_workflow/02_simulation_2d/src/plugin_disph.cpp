/**
 * @file plugin_disph.cpp
 * @brief 2D Shock Tube DISPH Plugin (V3 pure business logic)
 *
 * DISPH (Density-Independent SPH) configuration for 2D shock tube:
 * - Uses HLL Riemann solver with density-independent formulation
 * - Shock propagates in x-direction
 * - Mirror boundaries with ghost particles
 * - Physics-based parameter estimation
 * - V3 pure functional interface
 */

#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/boundaries/boundary_builder.hpp"
#include "core/boundaries/boundary_types.hpp"
#include "core/parameters/disph_parameters_builder.hpp"
#include "core/boundaries/ghost_particle_manager.hpp"
#include "core/parameters/parameter_estimator.hpp"
#include "core/parameters/parameter_validator.hpp"
#include "core/simulation/simulation.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/utilities/vector.hpp"
#include "exception.hpp"

#include <cmath>
#include <iostream>
#include <vector>

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
class DISPHShockTube2DPlugin : public SimulationPluginV3<2> {
public:
    std::string get_name() const override { return "disph_shock_tube_2d"; }

    std::string get_description() const override { return "2D Sod shock tube - DISPH (Density-Independent SPH)"; }

    std::string get_version() const override { return "1.0.0"; }

    std::vector<std::string> get_source_files() const override { return {"plugin_disph.cpp"}; }

    InitialCondition<2> create_initial_condition() const override {
        static constexpr int Dim = 2;

        std::cout << "\n=== 2D DISPH SHOCK TUBE ===\n";
        std::cout << "Algorithm: DISPH (Density-Independent SPH)\n\n";

        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================

        const real gamma = 1.4;  // Adiabatic index for ideal gas

        // Domain setup - Following 2D SPH literature recommendations
        // Reference: Puri & Ramachandran (2014), Price (2024)
        // X-direction: [0, 1.0] with discontinuity at x=0.5 (standard Sod setup)
        // Y-direction: [0, 0.1] small height for planar 2D (quasi-1D behavior)

        // SPH STRATEGY: Equal mass particles with variable spacing
        // - SPH convention: ALL particles have EQUAL MASS
        // - Density: ρ = Σ m W(r_ij, h) ∝ particle number density for equal mass
        // - Standard Sod: ρ_left/ρ_right = 1.0/0.125 = 8:1 density ratio
        // - In 2D: ρ ∝ 1/(dx*dy), with constant dy → ρ ∝ 1/dx
        // - Need spacing ratio: dx_right/dx_left = ρ_left/ρ_right = 8.0
        //
        // SPACING STRATEGY (from literature):
        // 1. Equal mass for all particles (SPH standard)
        // 2. Variable spacing: dx_right = 8.0 * dx_left
        // 3. Smoothing length scales with spacing: h ∝ dx

        const real Ly = 0.1;  // Small height for planar 2D (literature: 0.1-0.2)
        const real Lx_left = 0.5;   // Left region [0, 0.5]
        const real Lx_right = 0.5;  // Right region [0.5, 1.0]

        // Target densities (Sod standard)
        const real rho_target_left = 1.0;
        const real rho_target_right = 0.125;
        const real spacing_ratio = rho_target_left / rho_target_right;  // 8.0

        // Left region: fine spacing
        const int Nx_left = 200;  // High resolution for shock features
        const real dx_left_initial = Lx_left / Nx_left;
        
        // Right region: coarse spacing (8x larger for 8:1 density ratio)
        const real dx_right_initial = spacing_ratio * dx_left_initial;
        const int Nx_right = static_cast<int>(Lx_right / dx_right_initial);
        
        const real dy = dx_left_initial;  // Y-spacing matches left region
        const int Ny = static_cast<int>(Ly / dy);

        const int num = Nx_left * Ny + Nx_right * Ny;

        // EQUAL MASS - SPH convention
        const real mass = rho_target_left * dx_left_initial * dy;

        std::vector<SPHParticle<Dim>> particles(num);

        std::cout << "\n--- Particle Initialization (2D Planar Sod Shock Tube) ---\n";
        std::cout << "Reference: Puri & Ramachandran (2014), Price (2024)\n";
        std::cout << "Total particles: " << num << "\n";
        std::cout << "Grid: " << (Nx_left + Nx_right) << " × " << Ny << "\n";
        std::cout << "Algorithm: DISPH (Density-Independent SPH)\n";
        std::cout << "STRATEGY: Equal mass + 8:1 variable spacing\n";
        std::cout << "Domain: X=[0, 1.0], Y=[0, " << Ly << "] (planar 2D)\n";
        std::cout << "Target densities: left=" << rho_target_left << ", right=" << rho_target_right << "\n";
        std::cout << "Spacing: left=" << dx_left_initial << ", right=" << dx_right_initial 
                  << " (ratio=" << (dx_right_initial/dx_left_initial) << ")\n";
        std::cout << "Particle mass: m=" << mass << " (equal for all)\n";
        std::cout << "Discontinuity at x=0.5 (standard Sod setup)\n";

        // Initial smoothing length estimate
        constexpr real kappa = 1.2;
        const real sml_left_initial = kappa * dx_left_initial;
        const real sml_right_initial = kappa * dx_right_initial;

        std::cout << "Initial sml: left=" << sml_left_initial << ", right=" << sml_right_initial << "\n";

        int idx = 0;

        // Initialize left side particles (HIGH density region)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_left = 0.0 + dx_left_initial * 0.5;  // Start from x=0

            for (int i = 0; i < Nx_left; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_left, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = rho_target_left;  // Initial guess
                p.pres = 1.0;
                p.mass = mass;  // Equal mass (SPH convention)
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.sound = std::sqrt(gamma * p.pres / p.dens);
                p.sml = sml_left_initial;
                p.id = idx;

                x_left += dx_left_initial;
                ++idx;
            }
        }

        // Initialize right side particles (LOW density region)
        for (int j = 0; j < Ny; ++j) {
            real y = dy * (j + 0.5);
            real x_right = 0.5 + dx_right_initial * 0.5;  // Start from x=0.5

            for (int i = 0; i < Nx_right; ++i) {
                auto& p = particles[idx];
                p.pos = Vector<Dim>{x_right, y};
                p.vel = Vector<Dim>{0.0, 0.0};
                p.dens = rho_target_right;  // Initial guess
                p.pres = 0.1;
                p.mass = mass;  // Equal mass (SPH convention)
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.sound = std::sqrt(gamma * p.pres / p.dens);
                p.sml = sml_right_initial;
                p.id = idx;

                x_right += dx_right_initial;
                ++idx;
            }
        }

        // ============================================================
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================

        std::cout << "\n--- Parameter Estimation ---\n";

        const real dx_left_relaxed = dx_left_initial;
        const real dx_right_relaxed = dx_right_initial;

        auto config = ParameterEstimator::analyze_particle_config<Dim>(particles);

        std::cout << "Particle configuration:\n";
        std::cout << "  Spacing: left=" << dx_left_relaxed << ", right=" << dx_right_relaxed << "\n";
        std::cout << "  Max sound speed: " << config.max_sound_speed << "\n";
        std::cout << "  Estimated dimension: 2D\n";

        auto suggestions = ParameterEstimator::suggest_parameters<Dim>(particles, 2.0);

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

                                    // Transition to DISPH (Density-Independent SPH)
                                    .as_disph()
                                    .with_artificial_viscosity(
                                        1.0  // alpha: bulk viscosity coefficient (typical range: 0.5-2.0)
                                        // beta is deprecated - now derived as 2*alpha internally
                                        // use_balsara_switch defaults to true
                                        // use_time_dependent_av defaults to false
                                        )

                                    .build();

            std::cout << "✓ Parameters built with type-safe DISPH API\n";
            std::cout << "  - DISPH uses density-independent formulation\n";

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
                               .in_range(Vector<Dim>{-0.5, 0.0}, Vector<Dim>{1.5, 0.5})
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
                               .in_range(Vector<Dim>{0.0, 0.0}, Vector<Dim>{1.0, Ly})
                               .with_mirror_in_dimension(0, MirrorType::NO_SLIP, dx_left_relaxed,
                                                         dx_right_relaxed)  // X: walls at 0 and 1
                               .with_periodic_in_dimension(1)  // Y: periodic (planar symmetry)
                               .build();

            std::cout << "✓ Ghost particle system configured\n";
            std::cout << "  ✓ Type-safe declarative API\n";
            std::cout << "  ✓ X-direction: NO_SLIP (walls at x=0 and x=1)\n";
            std::cout << "  ✓ Y-direction: PERIODIC (planar symmetry)\n";
        }

        std::cout << "  X-boundary: MIRROR (NO_SLIP) [" << boundary_config.range_min[0] << ", "
                  << boundary_config.range_max[0] << "]\n";
        std::cout << "    Left spacing: dx=" << dx_left_relaxed << ", wall offset=" << (0.5 * dx_left_relaxed) << "\n";
        std::cout << "    Right spacing: dx=" << dx_right_relaxed << ", wall offset=" << (0.5 * dx_right_relaxed) << "\n";
        std::cout << "    Left wall position:  " << boundary_config.get_wall_position(0, false) << "\n";
        std::cout << "    Right wall position: " << boundary_config.get_wall_position(0, true) << "\n";
        std::cout << "  Y-boundary: PERIODIC [" << boundary_config.range_min[1] << ", "
                  << boundary_config.range_max[1] << "]\n";
        std::cout << "    Particle spacing: dy=" << dy << "\n";

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

DEFINE_SIMULATION_PLUGIN_V3(DISPHShockTube2DPlugin, 2)

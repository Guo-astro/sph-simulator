#include "core/simulation_plugin.hpp"
#include "core/sph_parameters_builder.hpp"
#include "core/parameter_estimator.hpp"
#include "core/parameter_validator.hpp"
#include "core/simulation.hpp"
#include "core/sph_particle.hpp"
#include "core/ghost_particle_manager.hpp"
#include "core/boundary_types.hpp"
#include "exception.hpp"
#include <vector>
#include <iostream>

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
class ShockTubePluginEnhanced : public SimulationPlugin {
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
    
    void initialize(std::shared_ptr<Simulation<DIM>> sim,
                   std::shared_ptr<SPHParameters> params) override {
        // This plugin is for 1D simulations
        static constexpr int Dim = 1;
        static_assert(DIM == Dim, "Shock tube requires DIM=1");

        std::cout << "\n=== ENHANCED SHOCK TUBE (Physics-Based Parameters) ===\n";
        
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        // Create particles FIRST so we can analyze their configuration
        // for physics-based parameter estimation
        // ============================================================
        // STEP 1: INITIALIZE PARTICLES
        // ============================================================
        // Sod shock tube with proper density ratio
        // Left:  x ∈ [-0.5, 0.5], ρ=1.0,   P=1.0
        // Right: x ∈ [0.5, 1.5],  ρ=0.125, P=0.1
        // For uniform mass and ρ = m/dx, need dx_L/dx_R = ρ_R/ρ_L = 1/8
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
        
        std::vector<SPHParticle<DIM>> particles(num);
        
        std::cout << "\n--- Particle Initialization ---\n";
        std::cout << "Total particles: " << num << " (" << N_left << " left + " << N_right << " right)\n";
        std::cout << "Left state:  ρ=1.0,    P=1.0,   dx=" << dx_left << "\n";
        std::cout << "Right state: ρ=0.125,  P=0.1,   dx=" << dx_right << " (Sod 1978)\n";
        std::cout << "Discontinuity at x=0.5\n";
        std::cout << "Uniform mass: m=" << mass << "\n";
        
        int idx = 0;
        
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
            p_i.id = idx;
            p_i.acc[0] = 0.0;
            
            x_right += dx_right;
            idx++;
        }
        
        // ============================================================
        // STEP 2: ESTIMATE PHYSICS-BASED PARAMETERS
        // ============================================================
        // Use ParameterEstimator to calculate CFL and neighbor_number
        // from actual particle configuration and stability theory
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
        // Use the physics-based suggestions in the builder
        // ============================================================
        
        if (params->time.end == 0) {
            std::cout << "\n--- Building Parameter Set ---\n";
            
            auto builder_params = SPHParametersBuilder()
                // Time configuration
                .with_time(
                    0.0,    // start time
                    0.15,    // end time  
                    0.01,   // output interval
                    0.01    // energy output interval
                )
                
                // SPH algorithm
                .with_sph_type("gsph")  // Godunov SPH for shock tube
                
                // Physics-based CFL (from stability analysis!)
                .with_cfl(
                    suggestions.cfl_sound,  // NOT hardcoded!
                    suggestions.cfl_force   // Based on theory!
                )
                
                // Physics parameters (using estimated neighbor number)
                .with_physics(
                    suggestions.neighbor_number,  // From kernel support
                    gamma                         // Adiabatic index
                )
                
                // Kernel type
                .with_kernel("cubic_spline")
                
                // Artificial viscosity for shock capturing
                .with_artificial_viscosity(
                    1.0,    // alpha
                    true,   // Balsara switch (reduces viscosity in shear)
                    false   // no time-dependent AV
                )
                
                // Tree parameters
                .with_tree_params(20, 1)
                
                // Iterative smoothing length
                .with_iterative_smoothing_length(true)
                
                .build();
            
            *params = *builder_params;
            
            std::cout << "✓ Parameters built with physics-based values\n";
        } else {
            std::cout << "\n--- Using Pre-Configured Parameters ---\n";
            std::cout << "(Loaded from JSON configuration)\n";
        }
        
        // ============================================================
        // STEP 4: VALIDATE PARAMETERS AGAINST PARTICLES
        // ============================================================
        // This catches mismatches that would cause simulation blow-up!
        // ============================================================
        
        std::cout << "\n--- Parameter Validation ---\n";
        
        try {
            ParameterValidator::validate_all(particles, params);
            std::cout << "✓ All parameters validated - SAFE to run!\n";
            
            // Show what timestep we'll get
            auto config = ParameterEstimator::analyze_particle_config(particles);
            real dt_sound = params->cfl.sound * config.avg_spacing / config.max_sound_speed;
            real dt_force = std::numeric_limits<real>::infinity();
            if (config.max_acceleration > 1e-10) {
                dt_force = params->cfl.force * std::sqrt(config.avg_spacing / config.max_acceleration);
            }
            
            std::cout << "\nExpected timestep:\n";
            std::cout << "  dt_sound = " << dt_sound << "\n";
            std::cout << "  dt_force = " << (dt_force < 1e100 ? std::to_string(dt_force) : "inf") << "\n";
            std::cout << "  dt_actual = " << std::min(dt_sound, dt_force) << "\n";
            
        } catch (const std::runtime_error& e) {
            std::cerr << "\n❌ VALIDATION FAILED!\n";
            std::cerr << e.what() << "\n";
            std::cerr << "\nSimulation will NOT run - parameters are unsafe!\n";
            THROW_ERROR("Parameter validation failed");
        }
        
        // ============================================================
        // ============================================================
        // STEP 5: SET PARTICLES IN SIMULATION
        // ============================================================
        
        const int num_particles = static_cast<int>(particles.size());
        sim->particle_num = num_particles;
        sim->particles = std::move(particles);
        
        // ============================================================
        // STEP 6: INITIALIZE GHOST PARTICLE SYSTEM
        // ============================================================
        // Modern boundary system using BoundaryConfiguration
        // This replaces the legacy periodic boundary parameters
        // 
        // For shock tube: Use MIRROR boundaries (reflective walls)
        // NOT periodic - we want walls, not wrapping!
        // ============================================================
        std::cout << "\n--- Ghost Particle System ---\n";
        
        // Configure mirror boundary with ghost particles (reflective walls)
        BoundaryConfiguration<DIM> ghost_config;
        ghost_config.is_valid = true;
        ghost_config.types[0] = BoundaryType::MIRROR;
        ghost_config.range_min[0] = -0.5;
        ghost_config.range_max[0] = 1.5;
        ghost_config.enable_lower[0] = true;
        ghost_config.enable_upper[0] = true;
        ghost_config.mirror_types[0] = MirrorType::NO_SLIP;
        
        // Initialize ghost particle manager
        sim->ghost_manager->initialize(ghost_config);
        
        // Set kernel support radius
        // At this point particles don't have sml calculated yet, so estimate from spacing
        // For 1D, typical sml = 2 * dx for cubic spline kernel
        // Use the larger spacing (right side) as conservative estimate
        const real estimated_sml = 2.0 * dx_right;
        const real kernel_support_radius = 2.0 * estimated_sml;  // 2h for cubic spline
        sim->ghost_manager->set_kernel_support_radius(kernel_support_radius);
        
        // Generate initial ghost particles
        sim->ghost_manager->generate_ghosts(sim->particles);
        
        std::cout << "✓ Ghost particle system initialized\n";
        std::cout << "  Boundary type: MIRROR (NO_SLIP)\n";
        std::cout << "  Domain range: [" << ghost_config.range_min[0] 
                  << ", " << ghost_config.range_max[0] << "]\n";
        std::cout << "  Estimated smoothing length: " << estimated_sml << "\n";
        std::cout << "  Kernel support radius: " << kernel_support_radius << "\n";
        std::cout << "  Generated " << sim->ghost_manager->get_ghost_count() 
                  << " ghost particles\n";
        
        std::cout << "\n--- Configuration Summary ---\n";
        std::cout << "SPH Algorithm: ";
        switch(params->type) {
            case SPHType::SSPH:  std::cout << "Standard SPH\n"; break;
            case SPHType::DISPH: std::cout << "Density Independent SPH\n"; break;
            case SPHType::GSPH:  std::cout << "Godunov SPH\n"; break;
        }
        std::cout << "CFL coefficients: sound=" << params->cfl.sound 
                  << ", force=" << params->cfl.force << "\n";
        std::cout << "Neighbor number: " << params->physics.neighbor_number << "\n";
        std::cout << "Gamma (adiabatic): " << params->physics.gamma << "\n";
        std::cout << "Kernel: ";
        switch(params->kernel) {
            case KernelType::CUBIC_SPLINE: std::cout << "Cubic Spline\n"; break;
            case KernelType::WENDLAND: std::cout << "Wendland\n"; break;
            case KernelType::UNKNOWN: std::cout << "Unknown\n"; break;
        }
        
        std::cout << "\n=== Initialization Complete ===\n\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube_plugin_enhanced.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN(ShockTubePluginEnhanced)

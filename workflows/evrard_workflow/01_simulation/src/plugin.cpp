#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include "parameters.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/boundaries/boundary_builder.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <iostream>
#include <vector>
#include <cmath>

namespace sph {

/**
 * @brief Evrard Collapse Test
 * 
 * 3D self-gravitating sphere collapse test.
 * Polytropic sphere with Γ=5/3 collapses under self-gravity.
 * 
 * Initial conditions:
 * - M = 1, R = 1
 * - ρ(r) ∝ 1/r
 * - u = 0.05G (thermal energy)
 * - Initially at rest
 * 
 * Reference: Evrard (1988)
 */
class EvrardPlugin : public SimulationPluginV3<3> {
public:
    std::string get_name() const override {
        return "evrard_collapse";
    }
    
    std::string get_description() const override {
        return "3D Evrard gravitational collapse (V3 pure functional interface)";
    }
    
    std::string get_version() const override {
        return "3.0.1";  // V3 migration
    }
    
    InitialCondition<3> create_initial_condition() const override {
        static constexpr int Dim = 3;
        static constexpr real G = 1.0;           // Gravitational constant
        static constexpr real gamma = 5.0 / 3.0; // Polytropic index
        static constexpr real u_thermal = 0.05;  // Thermal energy coefficient

        std::cout << "Initializing Evrard collapse (V3 functional interface)...\n";
        
        std::vector<SPHParticle<Dim>> particles;
        const int N = 20;  // Grid resolution
        const real dx = 2.0 / N;
        
        std::cout << "Creating sphere with N=" << N << "^3 grid...\n";
        
        // Create particles in a sphere
        for(int i = 0; i < N; ++i) {
            for(int j = 0; j < N; ++j) {
                for(int k = 0; k < N; ++k) {
                    Vector<Dim> r = {
                        (i + 0.5) * dx - 1.0,
                        (j + 0.5) * dx - 1.0,
                        (k + 0.5) * dx - 1.0
                    };
                    const real r_0 = abs(r);
                    if(r_0 > 1.0) {
                        continue;
                    }
                    
                    // Apply radial distortion for better sampling
                    if(r_0 > 0.0) {
                        const real r_abs = std::pow(r_0, 1.5);
                        r = r * (r_abs / r_0);
                    }
                    
                    SPHParticle<Dim> p_i;
                    p_i.pos = r;
                    particles.emplace_back(p_i);
                }
            }
        }
        
        const real mass = 1.0 / particles.size();
        const real u = u_thermal * G;
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
        std::cout << "  Thermal energy: " << u << "\n";
        
        // Set particle properties
        int i = 0;
        for(auto & p_i : particles) {
            // Zero velocity (initially at rest)
            for (int d = 0; d < Dim; ++d) {
                p_i.vel[d] = 0.0;
            }
            
            p_i.mass = mass;
            real r_mag = std::sqrt(inner_product(p_i.pos, p_i.pos));
            
            // Guard against division by zero at center
            constexpr real epsilon = 1.0e-10;
            if (r_mag < epsilon) {
                r_mag = epsilon;
            }
            
            // Evrard initial conditions: ρ(r) ∝ 1/r
            p_i.dens = 1.0 / (2.0 * M_PI * r_mag);
            p_i.ene = u;
            p_i.pres = (gamma - 1.0) * p_i.dens * u;
            p_i.id = i;
            
            // Debug initial state
            if (i < 5 || !std::isfinite(p_i.dens) || !std::isfinite(p_i.pres)) {
                std::cout << "  Particle " << i << ": pos=(" << p_i.pos[0] << "," 
                          << p_i.pos[1] << "," << p_i.pos[2] << "), r_mag=" << r_mag
                          << ", dens=" << p_i.dens << ", pres=" << p_i.pres 
                          << ", ene=" << p_i.ene << "\n";
            }
            
            ++i;
        }
        
        // ==================== TYPE-SAFE PARAMETER CONSTRUCTION ====================
        
        std::cout << "Building type-safe parameters...\n";
        
        std::shared_ptr<SPHParameters> param;
        try {
            param = SPHParametersBuilderBase()
                // Time parameters
                .with_time(
                    /*start=*/0.0,
                    /*end=*/60.0,
                    /*output=*/0.1,
                    /*energy=*/0.1  // Energy output interval
                )
                
                // CFL parameters (stability)
                .with_cfl(
                    /*sound=*/0.3,  // CFL for sound speed
                    /*force=*/0.25  // CFL for force term
                )
                
                // Physical parameters
                .with_physics(
                    /*neighbor_number=*/50,  // Target neighbors for kernel
                    /*gamma=*/gamma          // Adiabatic index
                )
                
                // Kernel function
                .with_kernel("cubic_spline")
                
                // ✅ GRAVITY - Type-safe, discoverable, validated
                .with_gravity(
                    /*constant=*/G,      // Gravitational constant
                    /*theta=*/0.5        // Barnes-Hut opening angle
                )
                
                // Tree construction parameters
                .with_tree_params(
                    /*max_level=*/20,           // Maximum tree depth
                    /*leaf_particle_num=*/1     // Particles per leaf
                )
                
                // Transition to Standard SPH
                .as_ssph()
                
                // SSPH requires artificial viscosity (type-enforced!)
                .with_artificial_viscosity(
                    /*alpha=*/1.0,                  // Viscosity coefficient
                    /*use_balsara_switch=*/true,    // Reduce viscosity in shear
                    /*use_time_dependent_av=*/false // Static viscosity
                )
                
                // Build and validate
                .build();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Parameter build failed: ") + e.what());
        }
        
        std::cout << "✓ Parameters validated and set:\n";
        std::cout << "  Algorithm: Standard SPH (SSPH)\n";
        std::cout << "  Gravity enabled: " << param->has_gravity() << "\n";
        
        // ✅ TYPE-SAFE: Pattern match on gravity variant
        if (param->has_gravity()) {
            const auto& grav = param->get_newtonian_gravity();
            std::cout << "  G = " << grav.constant << "\n";
            std::cout << "  θ (theta) = " << grav.theta << "\n";
        }
        
        std::cout << "  γ (gamma) = " << param->get_physics().gamma << "\n";
        std::cout << "  Artificial viscosity α = " << param->get_av().alpha << "\n";
        std::cout << "  Neighbor number = " << param->get_physics().neighbor_number << "\n";
        
        std::cout << "V3 initialization complete!\n";
        
        // Build boundary configuration (no boundaries for Evrard collapse)
        auto boundary_config = BoundaryBuilder<Dim>()
            .build();
        
        // Return V3 initial condition
        return InitialCondition<Dim>{
            .particles = std::move(particles),
            .parameters = param,
            .boundary_config = boundary_config
        };
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN_V3(sph::EvrardPlugin, 3)

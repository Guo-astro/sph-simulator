#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/plugins/initial_condition.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include "core/boundaries/boundary_builder.hpp"
#include "parameters.hpp"
#include "core/particles/sph_particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <iostream>
#include <vector>
#include <cmath>

namespace sph {

/**
 * @brief Kelvin-Helmholtz Instability Test
 * 
 * 2D shear flow instability test.
 * Two fluid layers with different velocities separated by a sharp interface.
 * Small perturbations grow into characteristic vortex structures.
 * 
 * Initial conditions:
 * - Upper/lower layers (y > 0.75 or y < 0.25): ρ=1, vx=-0.5
 * - Middle layer (0.25 < y < 0.75): ρ=2, vx=+0.5
 * - Sinusoidal velocity perturbation in vy
 * 
 * Reference: Springel (2010)
 */
class KHIPlugin : public SimulationPluginV3<2> {
public:
    std::string get_name() const override {
        return "kelvin_helmholtz_instability";
    }
    
    std::string get_description() const override {
        return "2D Kelvin-Helmholtz instability (V3 pure functional interface)";
    }
    
    std::string get_version() const override {
        return "2.0.1";  // V3 migration
    }
    
    InitialCondition<2> create_initial_condition() const override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;
        static constexpr real gamma = 5.0 / 3.0;

        std::cout << "Initializing Kelvin-Helmholtz instability (V3 functional interface)...\n";
        
        const int N = 128;  // Grid resolution
        const int num = N * N * 3 / 4;
        const real dx = 1.0 / N;
        const real mass = 1.5 / num;
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        // Velocity perturbation function
        auto vy = [](const real x, const real y) {
            constexpr real sigma2_inv = 2.0 / (0.05 * 0.05);
            return 0.1 * std::sin(4.0 * M_PI * x) * 
                   (std::exp(-sqr(y - 0.25) * 0.5 * sigma2_inv) + 
                    std::exp(-sqr(y - 0.75) * 0.5 * sigma2_inv));
        };
        
        real x = dx * 0.5;
        real y = dx * 0.5;
        int region = 1;
        bool odd = true;
        
        std::cout << "Creating " << num << " particles...\n";
        
        for(int i = 0; i < num; ++i) {
            auto & p_i = particles[i];
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.vel[0] = region == 1 ? -0.5 : 0.5;
            p_i.vel[1] = vy(x, y);
            p_i.mass = mass;
            p_i.dens = static_cast<real>(region);
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;
            
            x += region == 1 ? 2.0 * dx : dx;
            
            if(x > 1.0) {
                y += dx;
                
                if(y > 0.25 && y < 0.75) {
                    region = 2;
                } else {
                    region = 1;
                }
                
                if(region == 1) {
                    if(odd) {
                        odd = false;
                        x = dx * 1.5;
                    } else {
                        odd = true;
                        x = dx * 0.5;
                    }
                } else {
                    x = dx * 0.5;
                }
            }
        }
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
        
        // Build parameters with type-safe builder
        std::shared_ptr<SPHParameters> param;
        try {
            param = SPHParametersBuilderBase()
                .with_time(
                    /*start=*/0.0,
                    /*end=*/2.0,
                    /*output=*/0.05,
                    /*energy=*/0.05
                )
                .with_cfl(
                    /*sound=*/0.3,
                    /*force=*/0.25
                )
                .with_physics(
                    /*neighbor_number=*/50,
                    /*gamma=*/gamma
                )
                .with_kernel("cubic_spline")
                .as_ssph()
                .with_artificial_viscosity(
                    /*alpha=*/1.0,
                    /*use_balsara_switch=*/true,
                    /*use_time_dependent_av=*/false
                )
                .build();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Parameter build failed: ") + e.what());
        }
        
        // Build periodic boundary configuration
        Vector<Dim> domain_min = {0.0, 0.0};
        Vector<Dim> domain_max = {1.0, 1.0};
        
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(domain_min, domain_max)
            .build();
        
        std::cout << "V3 initialization complete!\n";
        
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

DEFINE_SIMULATION_PLUGIN_V3(sph::KHIPlugin, 2)

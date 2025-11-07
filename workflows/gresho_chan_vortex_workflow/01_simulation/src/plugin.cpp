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
 * @brief Gresho-Chan Vortex Test
 * 
 * 2D vortex in pressure equilibrium with centrifugal force.
 * Tests code's ability to maintain balance between pressure gradient
 * and centrifugal force without artificial diffusion.
 * 
 * Initial conditions:
 * - Azimuthal velocity profile with discontinuities at r=0.2, r=0.4
 * - Pressure adjusted to balance centrifugal force
 * - Uniform density ρ=1
 * 
 * Reference: Gresho & Chan (1990)
 */
class GreshoChanVortexPlugin : public SimulationPluginV3<2> {
private:
    static real vortex_velocity(const real r) {
        if(r < 0.2) {
            return 5.0 * r;
        } else if(r < 0.4) {
            return 2.0 - 5.0 * r;
        } else {
            return 0.0;
        }
    }
    
    static real vortex_pressure(const real r) {
        if(r < 0.2) {
            return 5.0 + 12.5 * r * r;
        } else if(r < 0.4) {
            return 9.0 + 12.5 * r * r - 20.0 * r + 4.0 * std::log(5.0 * r);
        } else {
            return 3.0 + 4.0 * std::log(2.0);
        }
    }

public:
    std::string get_name() const override {
        return "gresho_chan_vortex";
    }
    
    std::string get_description() const override {
        return "2D Gresho-Chan vortex in pressure equilibrium (V3 pure functional interface)";
    }
    
    std::string get_version() const override {
        return "2.0.1";  // V3 migration
    }
    
    InitialCondition<2> create_initial_condition() const override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;
        static constexpr real gamma = 5.0 / 3.0;

        std::cout << "Initializing Gresho-Chan vortex (V3 functional interface)...\n";
        
        const int N = 64;  // Grid resolution
        const real dx = 1.0 / N;
        const int num = N * N;
        const real mass = 1.0 / num;
        
        std::vector<SPHParticle<Dim>> particles(num);
        
        real x = -0.5 + dx * 0.5;
        real y = -0.5 + dx * 0.5;
        
        std::cout << "Creating " << num << " particles...\n";
        
        for(int i = 0; i < num; ++i) {
            auto & p_i = particles[i];
            
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            
            // Calculate radius from origin
            real r = std::sqrt(x*x + y*y);
            
            // Velocity in azimuthal direction
            real vel = vortex_velocity(r);
            if (r > 0.0) {
                Vector<Dim> dir = {-y, x};
                real dir_mag = abs(dir);
                p_i.vel = dir * (vel / dir_mag);
            } else {
                p_i.vel = Vector<Dim>{};
            }
            
            p_i.dens = 1.0;
            p_i.pres = vortex_pressure(r);
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;
            
            x += dx;
            if(x > 0.5) {
                x = -0.5 + dx * 0.5;
                y += dx;
            }
        }
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
        
        // Build parameters with type-safe builder
        std::shared_ptr<SPHParameters> param;
        try {
            param = SPHParametersBuilderBase()
                .with_time(0.0, 3.0, 0.1, 0.1)
                .with_cfl(0.3, 0.25)
                .with_physics(50, gamma)
                .with_kernel("cubic_spline")
                .as_ssph()
                .with_artificial_viscosity(1.0, true, false)
                .build();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Parameter build failed: ") + e.what());
        }
        
        // Build periodic boundary configuration
        Vector<Dim> domain_min = {-0.5, -0.5};
        Vector<Dim> domain_max = {0.5, 0.5};
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_periodic_boundaries()
            .in_range(domain_min, domain_max)
            .build();
        
        std::cout << "V3 initialization complete!\n";
        
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

DEFINE_SIMULATION_PLUGIN_V3(sph::GreshoChanVortexPlugin, 2)

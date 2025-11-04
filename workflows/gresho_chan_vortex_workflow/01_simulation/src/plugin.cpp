#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "parameters.hpp"
#include "core/sph_particle.hpp"
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
class GreshoChanVortexPlugin : public SimulationPlugin {
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
        return "2D Gresho-Chan vortex in pressure equilibrium";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation<DIM>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;
        static_assert(DIM == Dim, "Gresho-Chan vortex requires DIM=2");

        std::cout << "Initializing Gresho-Chan vortex...\n";
        
        const int N = 64;  // Grid resolution
        const real dx = 1.0 / N;
        const int num = N * N;
        const real mass = 1.0 / num;
        const real gamma = param->physics.gamma;
        
        std::vector<SPHParticle<DIM>> particles(num);
        
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
                Vector<DIM> dir = {-y, x};
                real dir_mag = abs(dir);
                p_i.vel = dir * (vel / dir_mag);
            } else {
                p_i.vel = Vector<DIM>{};
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
        
        sim->particles = particles;
        sim->particle_num = particles.size();
        
        // Set simulation parameters
        param->time.end = 3.0;
        param->time.output = 0.1;
        param->cfl.sound = 0.3;
        param->physics.neighbor_number = 50;
        param->periodic.is_valid = true;
        param->periodic.range_max[0] = 0.5;
        param->periodic.range_max[1] = 0.5;
        param->periodic.range_min[0] = -0.5;
        param->periodic.range_min[1] = -0.5;
        
        std::cout << "Initialization complete!\n";
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::GreshoChanVortexPlugin)

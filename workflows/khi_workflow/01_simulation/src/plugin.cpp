#include "core/plugins/simulation_plugin.hpp"
#include "core/simulation/simulation.hpp"
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
class KHIPlugin : public SimulationPlugin<2> {
public:
    std::string get_name() const override {
        return "kelvin_helmholtz_instability";
    }
    
    std::string get_description() const override {
        return "2D Kelvin-Helmholtz instability";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation<2>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;

        std::cout << "Initializing Kelvin-Helmholtz instability...\n";
        
        const int N = 128;  // Grid resolution
        const int num = N * N * 3 / 4;
        const real dx = 1.0 / N;
        const real mass = 1.5 / num;
        const real gamma = param->physics.gamma;
        
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
        
        sim->particles = particles;
        sim->particle_num = particles.size();
        
        // Set simulation parameters
        param->time.end = 2.0;
        param->time.output = 0.05;
        param->cfl.sound = 0.3;
        param->physics.neighbor_number = 50;
        param->periodic.is_valid = true;
        param->periodic.range_max[0] = 1.0;
        param->periodic.range_max[1] = 1.0;
        param->periodic.range_min[0] = 0.0;
        param->periodic.range_min[1] = 0.0;
        
        std::cout << "Initialization complete!\n";
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::KHIPlugin, 2)

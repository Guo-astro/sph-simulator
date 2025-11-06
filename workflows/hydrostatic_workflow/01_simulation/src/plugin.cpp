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
 * @brief Hydrostatic Equilibrium Test
 * 
 * 2D test with constant pressure but density contrast.
 * High-density region surrounded by low-density ambient medium.
 * Tests ability to maintain hydrostatic equilibrium.
 * 
 * Initial conditions:
 * - Inner region: ρ=4, P=2.5
 * - Outer region: ρ=1, P=2.5
 * - Zero velocity everywhere
 * 
 * Reference: Saitoh & Makino (2013)
 */
class HydrostaticPlugin : public SimulationPlugin<2> {
public:
    std::string get_name() const override {
        return "hydrostatic";
    }
    
    std::string get_description() const override {
        return "2D hydrostatic equilibrium test";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation<2>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;

        std::cout << "Initializing hydrostatic equilibrium test...\n";
        
        const int N = 32;
        const real dx1 = 0.5 / N;  // High-density region spacing
        const real dx2 = dx1 * 2.0;  // Low-density region spacing
        const real mass = 1.0 / (N * N);
        const real gamma = param->physics.gamma;
        int particle_id = 0;
        
        std::vector<SPHParticle<Dim>> particles;
        
        std::cout << "Creating high-density inner region...\n";
        // High-density region
        real x = -0.25 + dx1 * 0.5;
        real y = -0.25 + dx1 * 0.5;
        while(y < 0.25) {
            SPHParticle<Dim> p_i;
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.vel = Vector<Dim>{};
            p_i.mass = mass;
            p_i.dens = 4.0;
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = particle_id++;
            particles.push_back(p_i);
            
            x += dx1;
            if(x > 0.25) {
                x = -0.25 + dx1 * 0.5;
                y += dx1;
            }
        }
        
        int inner_count = particle_id;
        std::cout << "  Inner particles: " << inner_count << "\n";
        
        std::cout << "Creating low-density ambient region...\n";
        // Ambient low-density region
        x = -0.5 + dx2 * 0.5;
        y = -0.5 + dx2 * 0.5;
        while(y < 0.5) {
            SPHParticle<Dim> p_i;
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.vel = Vector<Dim>{};
            p_i.mass = mass;
            p_i.dens = 1.0;
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = particle_id++;
            particles.push_back(p_i);
            
            do {
                x += dx2;
                if(x > 0.5) {
                    x = -0.5 + dx2 * 0.5;
                    y += dx2;
                }
            } while(x > -0.25 && x < 0.25 && y > -0.25 && y < 0.25);
        }
        
        std::cout << "  Ambient particles: " << (particle_id - inner_count) << "\n";
        
        }
        }
        
        sim->particles = particles;
        sim->particle_num = particles.size();
        
        // Set simulation parameters
        param->time.end = 3.0;
        param->time.output = 0.1;
        param->cfl.sound = 0.3;
        param->physics.neighbor_number = 50;
        
        std::cout << "Initialization complete!\n";
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::HydrostaticPlugin, 2)

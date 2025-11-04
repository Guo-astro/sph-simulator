#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "parameters.hpp"
#include "core/sph_particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <cmath>

namespace sph {

/**
 * @brief Pairing Instability Test
 * 
 * 2D uniform grid with random perturbations.
 * Tests for pairing instability in SPH kernels where particles
 * may cluster into pairs rather than maintaining uniform distribution.
 * 
 * Initial conditions:
 * - Uniform density ρ=1
 * - Uniform pressure P=1
 * - Zero velocity with small random perturbations
 * 
 * Reference: Schuessler & Schmitt (1981), Monaghan (2002)
 */
class PairingInstabilityPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "pairing_instability";
    }
    
    std::string get_description() const override {
        return "2D pairing instability test";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation<DIM>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;
        static_assert(DIM == Dim, "Pairing instability test requires DIM=2");

        std::cout << "Initializing pairing instability test...\n";
        
        const int N = 32;
        const real dx = 1.0 / N;
        const real mass = 1.0 / (N * N);
        const real gamma = param->physics.gamma;
        const real dens = 1.0;
        const real pres = 1.0;
        
        // Random number generator for perturbations
        std::mt19937 engine(12345);
        std::uniform_real_distribution<real> dist(-0.05 * dx, 0.05 * dx);
        
        std::vector<SPHParticle<DIM>> particles;
        int particle_id = 0;
        
        std::cout << "Creating uniform grid with random perturbations...\n";
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                SPHParticle<DIM> p;
                
                // Grid position with perturbation
                p.pos[0] = (i + 0.5) * dx + dist(engine);
                p.pos[1] = (j + 0.5) * dx + dist(engine);
                
                // Zero velocity
                p.vel = Vector<DIM>{};
                
                // Uniform thermodynamics
                p.mass = mass;
                p.dens = dens;
                p.pres = pres;
                p.ene = pres / ((gamma - 1.0) * dens);
                
                p.id = particle_id++;
                particles.push_back(p);
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
        std::cout << "  Particle spacing: " << dx << "\n";
        std::cout << "  Perturbation: ±" << (0.05 * dx) << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::PairingInstabilityPlugin)

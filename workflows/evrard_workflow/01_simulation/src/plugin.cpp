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
class EvrardPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "evrard";
    }
    
    std::string get_description() const override {
        return "3D Evrard collapse test with self-gravity";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation<DIM>> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // This plugin is for 3D simulations
        static constexpr int Dim = 3;
        static_assert(DIM == Dim, "Evrard collapse requires DIM=3");

        std::cout << "Initializing Evrard collapse simulation...\n";
        
        const int N = 20;  // Grid resolution
        auto & particles = sim->particles;
        const real dx = 2.0 / N;
        
        std::cout << "Creating sphere with N=" << N << "^3 grid...\n";
        
        // Create particles in a sphere
        for(int i = 0; i < N; ++i) {
            for(int j = 0; j < N; ++j) {
                for(int k = 0; k < N; ++k) {
                    Vector<DIM> r = {
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
                    
                    SPHParticle<DIM> p_i;
                    p_i.pos = r;
                    particles.emplace_back(p_i);
                }
            }
        }
        
        const real mass = 1.0 / particles.size();
        const real gamma = param->physics.gamma;
        const real G = param->gravity.constant;
        const real u = 0.05 * G;
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle mass: " << mass << "\n";
        std::cout << "  Thermal energy: " << u << "\n";
        
        int i = 0;
        for(auto & p_i : particles) {
            // Zero velocity
            for (int d = 0; d < DIM; ++d) {
                p_i.vel[d] = 0.0;
            }
            
            p_i.mass = mass;
            real r_mag = std::sqrt(inner_product(p_i.pos, p_i.pos));
            p_i.dens = 1.0 / (2.0 * M_PI * r_mag);
            p_i.ene = u;
            p_i.pres = (gamma - 1.0) * p_i.dens * u;
            p_i.id = i++;
        }
        
        sim->particle_num = particles.size();
        
        // Set simulation parameters
        param->time.end = 3.0;
        param->time.output = 0.1;
        param->cfl.sound = 0.3;
        param->physics.neighbor_number = 50;
        param->gravity.is_valid = true;
        param->gravity.constant = 1.0;
        param->gravity.theta = 0.5;
        
        std::cout << "Initialization complete!\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

} // namespace sph

DEFINE_SIMULATION_PLUGIN(sph::EvrardPlugin)

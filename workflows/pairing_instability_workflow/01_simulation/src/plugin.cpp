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
class PairingInstabilityPlugin : public SimulationPluginV3<2> {
public:
    std::string get_name() const override {
        return "pairing_instability";
    }
    
    std::string get_description() const override {
        return "2D pairing instability test (V3 pure functional interface)";
    }
    
    std::string get_version() const override {
        return "2.0.1";  // V3 migration
    }
    
    InitialCondition<2> create_initial_condition() const override {
        // This plugin is for 2D simulations
        static constexpr int Dim = 2;
        static constexpr real gamma = 5.0 / 3.0;

        std::cout << "Initializing pairing instability test (V3 functional interface)...\n";
        
        const int N = 32;
        const real dx = 1.0 / N;
        const real mass = 1.0 / (N * N);
        const real dens = 1.0;
        const real pres = 1.0;
        
        // Random number generator for perturbations
        std::mt19937 engine(12345);
        std::uniform_real_distribution<real> dist(-0.05 * dx, 0.05 * dx);
        
        std::vector<SPHParticle<Dim>> particles;
        int particle_id = 0;
        
        std::cout << "Creating uniform grid with random perturbations...\n";
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                SPHParticle<Dim> p;
                
                // Grid position with perturbation
                p.pos[0] = (i + 0.5) * dx + dist(engine);
                p.pos[1] = (j + 0.5) * dx + dist(engine);
                
                // Zero velocity
                p.vel = Vector<Dim>{};
                
                // Uniform thermodynamics
                p.mass = mass;
                p.dens = dens;
                p.pres = pres;
                p.ene = pres / ((gamma - 1.0) * dens);
                
                p.id = particle_id++;
                particles.push_back(p);
            }
        }
        
        std::cout << "  Total particles: " << particles.size() << "\n";
        std::cout << "  Particle spacing: " << dx << "\n";
        std::cout << "  Perturbation: ±" << (0.05 * dx) << "\n";
        
        // Build parameters
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
        
        // No boundaries for this test
        auto boundary_config = BoundaryBuilder<Dim>()
            .with_no_boundaries()
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

DEFINE_SIMULATION_PLUGIN_V3(sph::PairingInstabilityPlugin, 2)

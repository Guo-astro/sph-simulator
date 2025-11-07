#pragma once

#include <memory>
#include <string>
#include <vector>
#include "initial_condition.hpp"

namespace sph {

/**
 * @brief Pure business logic plugin interface (V3)
 * 
 * This is the RECOMMENDED interface for new plugins.
 * 
 * Key Principles:
 * - Plugin expresses WHAT (physics), not HOW (system management)
 * - Returns data, doesn't manipulate simulation state
 * - No coupling to Simulation internals
 * - Easy to test (pure functions)
 * - Framework handles all system initialization
 * 
 * Comparison with previous versions:
 * 
 * V1 (Legacy): void initialize(shared_ptr<Simulation>, shared_ptr<Parameters>)
 *   ❌ No type safety
 *   ❌ Can access uninitialized state
 *   ❌ Mixed business logic and system management
 * 
 * V2 (Type-Safe): void initialize(UninitializedSimulation, shared_ptr<Parameters>)
 *   ✅ Type safety (can't access uninitialized state)
 *   ⚠️ Still mixed business logic and system management
 *   ⚠️ Plugin manually moves particles, sets counts, configures ghost manager
 * 
 * V3 (Pure): InitialCondition create_initial_condition() const
 *   ✅ Type safety
 *   ✅ Pure business logic (only returns data)
 *   ✅ No system coupling
 *   ✅ Easiest to test and understand
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class SimulationPluginV3 {
public:
    virtual ~SimulationPluginV3() = default;

    // ===== Metadata =====
    
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_version() const = 0;

    // ===== Core Functionality (PURE BUSINESS LOGIC) =====
    
    /**
     * @brief Create initial conditions for the simulation
     * 
     * Plugin responsibilities (WHAT to simulate):
     * 1. Define particle positions, velocities, masses
     * 2. Set initial densities, pressures, energies
     * 3. Configure SPH parameters (neighbor count, CFL, kernel, etc.)
     * 4. Specify boundary conditions (periodic, mirror, etc.)
     * 5. (Optional) Configure output settings
     * 
     * Framework responsibilities (HOW to initialize):
     * - Move particles into simulation
     * - Set particle count
     * - Initialize ghost particle manager
     * - Sync particle cache
     * - Build spatial tree
     * - Compute smoothing lengths
     * - Calculate initial densities from neighbors
     * - Compute initial forces
     * - Generate ghost particles
     * - Calculate initial timestep
     * 
     * Example:
     * ```cpp
     * InitialCondition<2> create_initial_condition() const override {
     *     // Create particles (business logic)
     *     auto particles = create_shock_tube_particles();
     *     
     *     // Configure parameters (business logic)
     *     auto params = SPHParametersBuilderBase()
     *         .with_time(0.0, 0.2, 0.01)
     *         .with_physics(15, 1.4)
     *         .as_ssph()
     *         .build();
     *     
     *     // Configure boundaries (business logic)
     *     auto boundaries = BoundaryBuilder<2>()
     *         .with_periodic_boundaries()
     *         .in_range({-0.5, 0.0}, {1.5, 0.5})
     *         .build();
     *     
     *     // Return data (framework handles system initialization)
     *     return InitialCondition<2>::with_particles(std::move(particles))
     *         .with_parameters(std::move(params))
     *         .with_boundaries(std::move(boundaries));
     * }
     * ```
     * 
     * @return Initial condition data
     */
    virtual InitialCondition<Dim> create_initial_condition() const = 0;

    // ===== Reproducibility =====
    
    virtual std::vector<std::string> get_source_files() const = 0;
};

// Export macros for dynamic loading
#if defined(_WIN32) || defined(__CYGWIN__)
    #define EXPORT_PLUGIN_API __declspec(dllexport)
#else
    #define EXPORT_PLUGIN_API __attribute__((visibility("default")))
#endif

/**
 * @brief Macro to define V3 plugin export functions
 * 
 * Use this for new plugins that use the pure business logic interface.
 * 
 * Example:
 *   class MyPlugin : public SimulationPluginV3<2> { ... };
 *   DEFINE_SIMULATION_PLUGIN_V3(MyPlugin, 2)
 */
#define DEFINE_SIMULATION_PLUGIN_V3(ClassName, Dimension) \
    extern "C" { \
        EXPORT_PLUGIN_API sph::SimulationPluginV3<Dimension>* create_plugin_v3() { \
            return new ClassName(); \
        } \
        EXPORT_PLUGIN_API void destroy_plugin_v3(sph::SimulationPluginV3<Dimension>* plugin) { \
            delete plugin; \
        } \
    }

} // namespace sph

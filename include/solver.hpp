#pragma once

#include <memory>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "defines.hpp"
#include "core/plugins/plugin_loader.hpp"  // For PluginDeleter type

namespace sph
{

struct SPHParameters;
template<int Dim> class Simulation;
template<int Dim> class Output;
template<int Dim> class Module;
template<int Dim> class PluginLoader;
template<int Dim> class SimulationPluginV3;

/**
 * @brief Main SPH simulation orchestrator using V3 pure plugin architecture.
 * 
 * The Solver class manages the simulation lifecycle:
 * - Loads V3 plugins that return InitialCondition data (.dylib/.so/.dll)
 * - Applies initial conditions to simulation (framework handles system details)
 * - Orchestrates time integration using appropriate SPH method (SSPH/DISPH/GSPH)
 * 
 * Usage:
 *   sph plugin.dylib [num_threads]
 * 
 * V3 Plugin Interface:
 *   - Plugins return InitialCondition (particles + parameters + boundaries)
 *   - Framework handles all system initialization automatically
 *   - Pure business logic - no system coupling
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 */
template<int Dim>
class Solver {
    // Core simulation components
    std::shared_ptr<SPHParameters>      m_param;
    std::shared_ptr<Output<Dim>>        m_output;
    std::string                         m_output_dir;
    std::shared_ptr<Simulation<Dim>>    m_sim;

    // SPH computation modules (type-specific)
    std::shared_ptr<Module<Dim>> m_timestep;
    std::shared_ptr<Module<Dim>> m_pre;        // Pre-interaction (density, etc.)
    std::shared_ptr<Module<Dim>> m_fforce;     // Fluid forces
    std::shared_ptr<Module<Dim>> m_gforce;     // Gravity forces

    // V3 Plugin system
    std::unique_ptr<PluginLoader<Dim>>                                    m_plugin_loader;
    std::unique_ptr<SimulationPluginV3<Dim>, typename PluginLoader<Dim>::PluginDeleter> m_plugin;
    std::string                                                            m_plugin_path;
    
    // Private methods
    void load_plugin();
    void make_initial_condition();
    void log_parameters();  // Log parameters after plugin configuration
    void initialize();
    void drift_half_step();
    void integrate();

public:
    Solver(int argc, char * argv[]);
    ~Solver();
    void run();
};

}
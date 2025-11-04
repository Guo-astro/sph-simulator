#pragma once

#include <memory>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "defines.hpp"
#include "core/plugin_loader.hpp"  // For PluginDeleter type

namespace sph
{

struct SPHParameters;
template<int Dim> class Simulation;
template<int Dim> class Output;
template<int Dim> class Module;
class PluginLoader;
class SimulationPlugin;

/**
 * @brief Main SPH simulation orchestrator using pure plugin architecture.
 * 
 * The Solver class manages the simulation lifecycle:
 * - Loads simulation setup from plugins (.dylib/.so/.dll)
 * - Reads SPH parameters from JSON configuration files
 * - Orchestrates time integration using appropriate SPH method (SSPH/DISPH/GSPH)
 * 
 * Usage:
 *   sph plugin.dylib [config.json]
 * 
 * @note This is a legacy class that uses DIM macro. Each plugin is compiled with specific DIM.
 */
class Solver {
    // Core simulation components
    std::shared_ptr<SPHParameters>      m_param;
    std::shared_ptr<Output<DIM>>        m_output;
    std::string                         m_output_dir;
    std::shared_ptr<Simulation<DIM>>    m_sim;

    // SPH computation modules (type-specific)
    std::shared_ptr<Module<DIM>> m_timestep;
    std::shared_ptr<Module<DIM>> m_pre;        // Pre-interaction (density, etc.)
    std::shared_ptr<Module<DIM>> m_fforce;     // Fluid forces
    std::shared_ptr<Module<DIM>> m_gforce;     // Gravity forces

    // Plugin system
    std::unique_ptr<PluginLoader>                    m_plugin_loader;
    std::unique_ptr<SimulationPlugin, PluginDeleter> m_plugin;
    std::string                                      m_plugin_path;
    
    // Private methods
    void read_parameterfile(const char * filename);
    void load_plugin();
    void make_initial_condition();
    void initialize();
    void predict();
    void correct();
    void integrate();

public:
    Solver(int argc, char * argv[]);
    void run();
};

}
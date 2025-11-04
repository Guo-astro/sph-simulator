#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sph {

// Forward declarations
class Simulation;
struct SPHParameters;

/**
 * @brief Base class for simulation plugins
 * 
 * Simulation plugins allow cases to be developed as self-contained modules
 * that can be dynamically loaded or statically linked.
 */
class SimulationPlugin {
public:
    virtual ~SimulationPlugin() = default;

    // Metadata
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_version() const = 0;

    // Core functionality
    virtual void initialize(std::shared_ptr<Simulation> sim,
                          std::shared_ptr<SPHParameters> params) = 0;

    // Reproducibility - return list of source files for archiving
    virtual std::vector<std::string> get_source_files() const = 0;
};

// Export macros for dynamic loading
#if defined(_WIN32) || defined(__CYGWIN__)
    #define EXPORT_PLUGIN_API __declspec(dllexport)
#else
    #define EXPORT_PLUGIN_API __attribute__((visibility("default")))
#endif

// Macro to define plugin export functions
#define DEFINE_SIMULATION_PLUGIN(ClassName) \
    extern "C" { \
        EXPORT_PLUGIN_API sph::SimulationPlugin* create_plugin() { \
            return new ClassName(); \
        } \
        EXPORT_PLUGIN_API void destroy_plugin(sph::SimulationPlugin* plugin) { \
            delete plugin; \
        } \
    }

} // namespace sph

/**
 * @file plugin_loader.hpp
 * @brief Dynamic plugin loading system for SPH simulations
 * 
 * Provides functionality to load simulation plugins from shared libraries (.so, .dylib, .dll)
 * and manage their lifecycle.
 */

#pragma once

#include <memory>
#include <string>
#include <stdexcept>

namespace sph {

// Forward declaration
class SimulationPlugin;

/**
 * @brief Exception thrown when plugin loading fails
 */
class PluginLoadError : public std::runtime_error {
public:
    explicit PluginLoadError(const std::string& message)
        : std::runtime_error(message) {}
};

// Function pointer types
typedef SimulationPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(SimulationPlugin*);

/**
 * @brief Custom deleter for unique_ptr that uses the plugin's destroy function
 */
struct PluginDeleter {
    DestroyPluginFunc destroy_func;
    
    void operator()(SimulationPlugin* plugin) {
        if (plugin && destroy_func) {
            destroy_func(plugin);
        }
    }
};

/**
 * @brief Manages dynamic loading of simulation plugins
 * 
 * Example usage:
 * @code
 * PluginLoader loader("path/to/plugin.dylib");
 * if (loader.is_loaded()) {
 *     auto plugin = loader.create_plugin();
 *     plugin->initialize(sim, params);
 * }
 * @endcode
 */
class PluginLoader {
public:
    /**
     * @brief Load a plugin from the specified path
     * @param plugin_path Path to the shared library (.so, .dylib, .dll)
     * @throws PluginLoadError if the library cannot be loaded
     */
    explicit PluginLoader(const std::string& plugin_path);
    
    /**
     * @brief Destructor - unloads the plugin library
     */
    ~PluginLoader();
    
    // Disable copying
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
    
    // Enable moving
    PluginLoader(PluginLoader&& other) noexcept;
    PluginLoader& operator=(PluginLoader&& other) noexcept;
    
    /**
     * @brief Check if the plugin library is successfully loaded
     * @return true if loaded, false otherwise
     */
    bool is_loaded() const noexcept;
    
    /**
     * @brief Get the last error message
     * @return Error description or empty string if no error
     */
    std::string get_error() const noexcept;
    
    /**
     * @brief Create a new instance of the plugin
     * @return Unique pointer to the plugin instance with custom deleter
     * @throws PluginLoadError if plugin creation fails
     */
    std::unique_ptr<SimulationPlugin, PluginDeleter> create_plugin();
    
    /**
     * @brief Get the path to the loaded plugin
     * @return Plugin library path
     */
    const std::string& get_plugin_path() const noexcept;

private:
    void* m_handle;                ///< Handle to the loaded library
    std::string m_plugin_path;     ///< Path to the plugin library
    std::string m_error;           ///< Last error message
    
    CreatePluginFunc m_create_func;
    DestroyPluginFunc m_destroy_func;
    
    /**
     * @brief Load function pointers from the library
     */
    void load_functions();
    
    /**
     * @brief Unload the plugin library
     */
    void unload();
};

} // namespace sph

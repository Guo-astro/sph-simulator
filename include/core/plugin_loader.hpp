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
#include <filesystem>
#include <sstream>
#include <dlfcn.h>  // POSIX dynamic loading

namespace sph {

// Forward declaration
template<int Dim> class SimulationPlugin;

/**
 * @brief Exception thrown when plugin loading fails
 */
class PluginLoadError : public std::runtime_error {
public:
    explicit PluginLoadError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Manages dynamic loading of simulation plugins
 * 
 * @tparam Dim Spatial dimension (1, 2, or 3)
 * 
 * Example usage:
 * @code
 * PluginLoader<1> loader("path/to/plugin.dylib");
 * if (loader.is_loaded()) {
 *     auto plugin = loader.create_plugin();
 *     plugin->initialize(sim, params);
 * }
 * @endcode
 */
template<int Dim>
class PluginLoader {
public:
    // Function pointer types
    using CreatePluginFunc = SimulationPlugin<Dim>* (*)();
    using DestroyPluginFunc = void (*)(SimulationPlugin<Dim>*);

    /**
     * @brief Custom deleter for unique_ptr that uses the plugin's destroy function
     */
    struct PluginDeleter {
        DestroyPluginFunc destroy_func;
        
        void operator()(SimulationPlugin<Dim>* plugin) {
            if (plugin && destroy_func) {
                destroy_func(plugin);
            }
        }
    };

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
    std::unique_ptr<SimulationPlugin<Dim>, PluginDeleter> create_plugin();
    
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

// Template implementation
template<int Dim>
PluginLoader<Dim>::PluginLoader(const std::string& plugin_path)
    : m_handle(nullptr)
    , m_plugin_path(plugin_path)
    , m_create_func(nullptr)
    , m_destroy_func(nullptr)
{
    // Convert to absolute path if relative
    std::filesystem::path path(plugin_path);
    if (path.is_relative()) {
        path = std::filesystem::current_path() / path;
    }
    m_plugin_path = path.string();
    
    // Load the library
    m_handle = dlopen(m_plugin_path.c_str(), RTLD_LAZY);
    
    if (!m_handle) {
        m_error = dlerror();
        return;
    }
    
    // Load function pointers
    try {
        load_functions();
    } catch (const PluginLoadError& e) {
        m_error = e.what();
        unload();
    }
}

template<int Dim>
PluginLoader<Dim>::~PluginLoader() {
    unload();
}

template<int Dim>
PluginLoader<Dim>::PluginLoader(PluginLoader&& other) noexcept
    : m_handle(other.m_handle)
    , m_plugin_path(std::move(other.m_plugin_path))
    , m_error(std::move(other.m_error))
    , m_create_func(other.m_create_func)
    , m_destroy_func(other.m_destroy_func)
{
    other.m_handle = nullptr;
    other.m_create_func = nullptr;
    other.m_destroy_func = nullptr;
}

template<int Dim>
PluginLoader<Dim>& PluginLoader<Dim>::operator=(PluginLoader&& other) noexcept {
    if (this != &other) {
        unload();
        
        m_handle = other.m_handle;
        m_plugin_path = std::move(other.m_plugin_path);
        m_error = std::move(other.m_error);
        m_create_func = other.m_create_func;
        m_destroy_func = other.m_destroy_func;
        
        other.m_handle = nullptr;
        other.m_create_func = nullptr;
        other.m_destroy_func = nullptr;
    }
    return *this;
}

template<int Dim>
bool PluginLoader<Dim>::is_loaded() const noexcept {
    return m_handle != nullptr && m_create_func != nullptr && m_destroy_func != nullptr;
}

template<int Dim>
std::string PluginLoader<Dim>::get_error() const noexcept {
    return m_error;
}

template<int Dim>
std::unique_ptr<SimulationPlugin<Dim>, typename PluginLoader<Dim>::PluginDeleter> 
PluginLoader<Dim>::create_plugin() {
    if (!is_loaded()) {
        throw PluginLoadError("Plugin not loaded: " + m_error);
    }
    
    // Clear any existing error
    dlerror();
    
    // Create plugin instance
    SimulationPlugin<Dim>* plugin = m_create_func();
    
    if (!plugin) {
        const char* error = dlerror();
        std::string err_msg = error ? error : "Unknown error creating plugin";
        throw PluginLoadError("Failed to create plugin instance: " + err_msg);
    }
    
    // Return with custom deleter
    return std::unique_ptr<SimulationPlugin<Dim>, PluginDeleter>(plugin, PluginDeleter{m_destroy_func});
}

template<int Dim>
const std::string& PluginLoader<Dim>::get_plugin_path() const noexcept {
    return m_plugin_path;
}

template<int Dim>
void PluginLoader<Dim>::load_functions() {
    // Clear any existing error
    dlerror();
    
    // Load create_plugin function
    m_create_func = reinterpret_cast<CreatePluginFunc>(dlsym(m_handle, "create_plugin"));
    const char* error = dlerror();
    if (error) {
        std::ostringstream oss;
        oss << "Failed to load 'create_plugin' function: " << error;
        throw PluginLoadError(oss.str());
    }
    
    // Load destroy_plugin function
    m_destroy_func = reinterpret_cast<DestroyPluginFunc>(dlsym(m_handle, "destroy_plugin"));
    error = dlerror();
    if (error) {
        std::ostringstream oss;
        oss << "Failed to load 'destroy_plugin' function: " << error;
        throw PluginLoadError(oss.str());
    }
}

template<int Dim>
void PluginLoader<Dim>::unload() {
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
        m_create_func = nullptr;
        m_destroy_func = nullptr;
    }
}

} // namespace sph

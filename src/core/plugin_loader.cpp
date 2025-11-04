/**
 * @file plugin_loader.cpp
 * @brief Implementation of dynamic plugin loading system
 */

#include "core/plugin_loader.hpp"
#include "core/simulation_plugin.hpp"
#include <dlfcn.h>  // POSIX dynamic loading
#include <filesystem>
#include <sstream>

namespace sph {

PluginLoader::PluginLoader(const std::string& plugin_path)
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

PluginLoader::~PluginLoader() {
    unload();
}

PluginLoader::PluginLoader(PluginLoader&& other) noexcept
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

PluginLoader& PluginLoader::operator=(PluginLoader&& other) noexcept {
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

bool PluginLoader::is_loaded() const noexcept {
    return m_handle != nullptr && m_create_func != nullptr && m_destroy_func != nullptr;
}

std::string PluginLoader::get_error() const noexcept {
    return m_error;
}

std::unique_ptr<SimulationPlugin, PluginDeleter> PluginLoader::create_plugin() {
    if (!is_loaded()) {
        throw PluginLoadError("Plugin not loaded: " + m_error);
    }
    
    // Clear any existing error
    dlerror();
    
    // Create plugin instance
    SimulationPlugin* plugin = m_create_func();
    
    if (!plugin) {
        const char* error = dlerror();
        std::string err_msg = error ? error : "Unknown error creating plugin";
        throw PluginLoadError("Failed to create plugin instance: " + err_msg);
    }
    
    // Return with custom deleter using the correct unique_ptr constructor
    return std::unique_ptr<SimulationPlugin, PluginDeleter>(plugin, PluginDeleter{m_destroy_func});
}

const std::string& PluginLoader::get_plugin_path() const noexcept {
    return m_plugin_path;
}

void PluginLoader::load_functions() {
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

void PluginLoader::unload() {
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
        m_create_func = nullptr;
        m_destroy_func = nullptr;
    }
}

} // namespace sph

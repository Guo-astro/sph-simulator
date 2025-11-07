#include <iostream>
#include <memory>
#include <dlfcn.h>
#include "include/core/plugins/simulation_plugin_v3.hpp"

int main() {
    std::cout << "Loading plugin...\n";
    
    void* handle = dlopen("workflows/shock_tube_workflow/02_simulation_2d/lib/libshock_tube_2d_ssph_plugin.dylib", RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << dlerror() << "\n";
        return 1;
    }
    
    std::cout << "Plugin loaded\n";
    
    typedef sph::SimulationPluginV3<2>* (*create_func)();
    auto create = (create_func)dlsym(handle, "create_simulation_plugin_v3");
    if (!create) {
        std::cerr << "Failed to find create function: " << dlerror() << "\n";
        dlclose(handle);
        return 1;
    }
    
    std::cout << "Creating plugin instance...\n";
    auto plugin = create();
    
    std::cout << "Plugin name: " << plugin->get_name() << "\n";
    std::cout << "Calling create_initial_condition...\n";
    
    auto ic = plugin->create_initial_condition();
    
    std::cout << "Success! Got " << ic.particle_count() << " particles\n";
    
    typedef void (*destroy_func)(sph::SimulationPluginV3<2>*);
    auto destroy = (destroy_func)dlsym(handle, "destroy_simulation_plugin_v3");
    if (destroy) {
        destroy(plugin);
    }
    
    dlclose(handle);
    
    std::cout << "Test complete\n";
    return 0;
}

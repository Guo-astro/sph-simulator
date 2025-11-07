// Use 1D for tests by default
static constexpr int Dim = 1;

#include <gtest/gtest.h>
#include "../../helpers/bdd_helpers.hpp"
#include "core/plugins/plugin_loader.hpp"
#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/simulation/simulation.hpp"
#include "parameters.hpp"
#include <memory>
#include <string>

/**
 * @file plugin_loader_test.cpp
 * @brief BDD-style tests for dynamic plugin loading system (V3 interface)
 * 
 * Feature: Plugin Loading System
 * As a simulation developer
 * I want to dynamically load simulation configurations as plugins
 * So that I can create self-contained, reproducible simulation workflows
 */

using namespace sph;

FEATURE(PluginLoaderFeature) {
    
    SCENARIO(CanLoadDynamicLibrary, LoadsSharedLibrary) {
        GIVEN("A valid plugin library path") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin.dylib";
            
            WHEN("We create a plugin loader with this path") {
                PluginLoader<Dim> loader(plugin_path);
                
                THEN("The loader should successfully load the library") {
                    EXPECT_TRUE(loader.is_loaded()) << "Failed to load plugin: " << loader.get_error();
                }
            }
        }
    }
    
    SCENARIO(HandlesMissingLibrary, FailsGracefully) {
        GIVEN("An invalid plugin library path") {
            std::string invalid_path = "non_existent_plugin.dylib";
            
            WHEN("We attempt to load the plugin") {
                PluginLoader<Dim> loader(invalid_path);
                
                THEN("The loader should fail to load") {
                    EXPECT_FALSE(loader.is_loaded());
                }
                
                AND("It should provide an error message") {
                    EXPECT_FALSE(loader.get_error().empty());
                }
            }
        }
    }
    
    SCENARIO(CreatesPluginInstance, InstantiatesPlugin) {
        GIVEN("A loaded plugin library") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            PluginLoader<Dim> loader(plugin_path);
            ASSERT_TRUE(loader.is_loaded());
            
            WHEN("We create a plugin instance (V3)") {
                auto plugin = loader.create_plugin_v3();
                
                THEN("The plugin should be created successfully") {
                    EXPECT_NE(plugin, nullptr);
                }
                
                AND("The plugin should have metadata") {
                    EXPECT_FALSE(plugin->get_name().empty());
                    EXPECT_FALSE(plugin->get_description().empty());
                    EXPECT_FALSE(plugin->get_version().empty());
                }
                
                AND("The plugin name should be 'shock_tube_enhanced'") {
                    EXPECT_EQ(plugin->get_name(), "shock_tube_enhanced");
                }
            }
        }
    }
    
    SCENARIO(InitializesSimulation, ConfiguresSimulationState) {
        GIVEN("A plugin instance (V3)") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            PluginLoader<Dim> loader(plugin_path);
            auto plugin = loader.create_plugin_v3();
            ASSERT_NE(plugin, nullptr);
            
            WHEN("We call create_initial_condition") {
                auto init_cond = plugin->create_initial_condition();
                
                THEN("The initial condition should have particles") {
                    EXPECT_GT(init_cond.particles.size(), 0);
                }
                
                AND("The parameters should be configured") {
                    EXPECT_GT(init_cond.parameters->get_time().end, 0.0);
                }
                
                AND("Particles should have valid physical properties") {
                    const auto& particles = init_cond.particles;
                    for (const auto& p : particles) {
                        EXPECT_GT(p.dens, 0.0) << "Density must be positive";
                        EXPECT_GT(p.mass, 0.0) << "Mass must be positive";
                        EXPECT_GE(p.pres, 0.0) << "Pressure must be non-negative";
                        EXPECT_TRUE(test_helpers::is_finite(p.ene)) << "Energy must be finite";
                    }
                }
                
                AND("Boundary configuration should be valid") {
                    EXPECT_TRUE(init_cond.boundary_config.is_valid);
                }
            }
        }
    }
    
    SCENARIO(ManagesPluginLifetime, CleansUpResources) {
        GIVEN("A plugin loader") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            
            WHEN("We create and destroy plugin instances (V3)") {
                PluginLoader<Dim> loader(plugin_path);
                auto plugin1 = loader.create_plugin_v3();
                auto plugin2 = loader.create_plugin_v3();
                
                THEN("Multiple instances can be created") {
                    EXPECT_NE(plugin1, nullptr);
                    EXPECT_NE(plugin2, nullptr);
                    EXPECT_NE(plugin1.get(), plugin2.get());
                }
                
                AND("Instances can be destroyed safely") {
                    plugin1.reset();
                    plugin2.reset();
                    EXPECT_EQ(plugin1, nullptr);
                    EXPECT_EQ(plugin2, nullptr);
                }
            }
        }
    }
    
    SCENARIO(SupportsRelativePaths, ResolvesPathsCorrectly) {
        GIVEN("A relative plugin path from project root") {
            std::string relative_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            
            WHEN("We load the plugin with relative path") {
                PluginLoader<Dim> loader(relative_path);
                
                THEN("The plugin should load successfully") {
                    EXPECT_TRUE(loader.is_loaded());
                }
            }
        }
    }
}

// Edge cases and error handling
FEATURE(PluginLoaderEdgeCases) {
    
    SCENARIO(HandlesRepeatedLoading, LoadsMultipleTimes) {
        GIVEN("A plugin path") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            
            WHEN("We load the same plugin multiple times") {
                PluginLoader<Dim> loader1(plugin_path);
                PluginLoader<Dim> loader2(plugin_path);
                
                THEN("Both loaders should work independently") {
                    EXPECT_TRUE(loader1.is_loaded());
                    EXPECT_TRUE(loader2.is_loaded());
                    
                    auto plugin1 = loader1.create_plugin_v3();
                    auto plugin2 = loader2.create_plugin_v3();
                    
                    EXPECT_NE(plugin1, nullptr);
                    EXPECT_NE(plugin2, nullptr);
                }
            }
        }
    }
}

// Integration tests
FEATURE(PluginLoaderIntegration) {
    
    SCENARIO(WorksWithSolver, IntegratesWithMainWorkflow) {
        GIVEN("A V3 plugin-based solver configuration") {
            std::string plugin_path = "../workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin_enhanced.dylib";
            
            WHEN("We load and initialize through V3 pattern") {
                PluginLoader<Dim> loader(plugin_path);
                auto plugin = loader.create_plugin_v3();
                
                auto init_cond = plugin->create_initial_condition();
                
                THEN("The configuration should be complete for simulation") {
                    EXPECT_GT(init_cond.particles.size(), 0);
                    EXPECT_GT(init_cond.parameters->get_time().end, 0.0);
                    EXPECT_GT(init_cond.parameters->get_physics().gamma, 0.0);
                    EXPECT_GT(init_cond.parameters->get_physics().neighbor_number, 0);
                }
                
                AND("Boundary configuration should be present") {
                    EXPECT_TRUE(init_cond.boundary_config.is_valid);
                }
            }
        }
    }
}

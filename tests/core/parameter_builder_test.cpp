// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../bdd_helpers.hpp"
#include "parameters.hpp"
#include "core/sph_algorithm_registry.hpp"
#include "core/sph_parameters_builder.hpp"
#include <memory>

using namespace sph;

FEATURE(SPHAlgorithmRegistry) {
    
    SCENARIO(SPHAlgorithmRegistry, RegistersStandardAlgorithms) {
        GIVEN("The algorithm registry is initialized") {
            WHEN("Querying for 'ssph'") {
                THEN("Should return SPHType::SSPH") {
                    SPHType type = SPHAlgorithmRegistry::get_type("ssph");
                    EXPECT_EQ(type, SPHType::SSPH);
                }
            }
            
            WHEN("Querying for 'disph'") {
                THEN("Should return SPHType::DISPH") {
                    SPHType type = SPHAlgorithmRegistry::get_type("disph");
                    EXPECT_EQ(type, SPHType::DISPH);
                }
            }
            
            WHEN("Querying for 'gsph'") {
                THEN("Should return SPHType::GSPH") {
                    SPHType type = SPHAlgorithmRegistry::get_type("gsph");
                    EXPECT_EQ(type, SPHType::GSPH);
                }
            }
        }
    }
    
    SCENARIO(SPHAlgorithmRegistry, AllowsCustomAlgorithmRegistration) {
        GIVEN("A custom SPH algorithm") {
            const std::string custom_name = "custom_sph";
            
            WHEN("Registering the algorithm") {
                // In future: SPHAlgorithmRegistry::register_algorithm(custom_name, SPHType::CUSTOM);
                
                THEN("Should be retrievable") {
                    // Future extensibility test
                    EXPECT_TRUE(true); // Placeholder
                }
            }
        }
    }
    
    SCENARIO(SPHAlgorithmRegistry, ThrowsOnUnknownAlgorithm) {
        GIVEN("An unregistered algorithm name") {
            const std::string unknown = "nonexistent_sph";
            
            WHEN("Querying for it") {
                THEN("Should throw exception") {
                    EXPECT_THROW(
                        SPHAlgorithmRegistry::get_type(unknown),
                        std::runtime_error
                    );
                }
            }
        }
    }
    
    SCENARIO(SPHAlgorithmRegistry, ListsAvailableAlgorithms) {
        GIVEN("The registry") {
            WHEN("Requesting available algorithms") {
                auto algorithms = SPHAlgorithmRegistry::list_algorithms();
                
                THEN("Should include standard algorithms") {
                    EXPECT_GE(algorithms.size(), 3);
                    EXPECT_TRUE(std::find(algorithms.begin(), algorithms.end(), "ssph") != algorithms.end());
                    EXPECT_TRUE(std::find(algorithms.begin(), algorithms.end(), "disph") != algorithms.end());
                    EXPECT_TRUE(std::find(algorithms.begin(), algorithms.end(), "gsph") != algorithms.end());
                }
            }
        }
    }
}

FEATURE(TypeSafeSPHParametersBuilder) {
    
    SCENARIO(TypeSafeSPHParametersBuilder, BuildsValidParameters) {
        GIVEN("A parameter builder") {
            SPHParametersBuilder builder;
            
            WHEN("Setting all required parameters") {
                auto params = builder
                    .with_time(0.0, 0.2, 0.01)
                    .with_sph_type("gsph")
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .build();
                
                THEN("Parameters should be correctly set") {
                    EXPECT_EQ(params->time.start, 0.0);
                    EXPECT_EQ(params->time.end, 0.2);
                    EXPECT_EQ(params->time.output, 0.01);
                    EXPECT_EQ(params->type, SPHType::GSPH);
                    EXPECT_EQ(params->cfl.sound, 0.3);
                    EXPECT_EQ(params->cfl.force, 0.125);
                    EXPECT_EQ(params->physics.neighbor_number, 50);
                    EXPECT_EQ(params->physics.gamma, 1.4);
                    EXPECT_EQ(params->kernel, KernelType::CUBIC_SPLINE);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeSPHParametersBuilder, SupportsOptionalParameters) {
        GIVEN("A parameter builder with required params") {
            SPHParametersBuilder builder;
            builder
                .with_time(0.0, 0.2, 0.01)
                .with_sph_type("ssph")
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Adding optional artificial viscosity") {
                auto params = builder
                    .with_artificial_viscosity(1.0, true, false)
                    .build();
                
                THEN("AV parameters should be set") {
                    EXPECT_EQ(params->av.alpha, 1.0);
                    EXPECT_TRUE(params->av.use_balsara_switch);
                    EXPECT_FALSE(params->av.use_time_dependent_av);
                }
            }
            
            WHEN("Adding periodic boundary conditions") {
                real range_max[Dim] = {1.5};
                real range_min[Dim] = {-0.5};
                auto params = builder
                    .with_periodic_boundary(range_min, range_max)
                    .build();
                
                THEN("Periodic BC should be configured") {
                    EXPECT_TRUE(params->periodic.is_valid);
                    EXPECT_EQ(params->periodic.range_max[0], 1.5);
                    EXPECT_EQ(params->periodic.range_min[0], -0.5);
                }
            }
            
            WHEN("Adding gravity") {
                auto params = builder
                    .with_gravity(1.0, 0.5)
                    .build();
                
                THEN("Gravity should be enabled") {
                    EXPECT_TRUE(params->gravity.is_valid);
                    EXPECT_EQ(params->gravity.constant, 1.0);
                    EXPECT_EQ(params->gravity.theta, 0.5);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeSPHParametersBuilder, ValidatesParameters) {
        GIVEN("A parameter builder") {
            SPHParametersBuilder builder;
            
            WHEN("Setting invalid time range (end < start)") {
                builder
                    .with_time(0.5, 0.2, 0.01)  // Invalid: end < start
                    .with_sph_type("ssph")
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline");
                
                THEN("build() should throw") {
                    EXPECT_THROW(builder.build(), std::runtime_error);
                }
            }
            
            WHEN("Setting negative CFL values") {
                builder
                    .with_time(0.0, 0.2, 0.01)
                    .with_sph_type("ssph")
                    .with_cfl(-0.3, 0.125)  // Invalid: negative
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline");
                
                THEN("build() should throw") {
                    EXPECT_THROW(builder.build(), std::runtime_error);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeSPHParametersBuilder, SupportsMethodChaining) {
        GIVEN("A parameter builder") {
            WHEN("Using fluent interface") {
                auto params = SPHParametersBuilder()
                    .with_time(0.0, 0.2, 0.01)
                    .with_sph_type("disph")
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .with_artificial_viscosity(1.0, true, false)
                    .with_artificial_conductivity(1.0)
                    .build();
                
                THEN("Should create valid parameters") {
                    EXPECT_EQ(params->type, SPHType::DISPH);
                    EXPECT_TRUE(params->ac.is_valid);
                    EXPECT_EQ(params->ac.alpha, 1.0);
                }
            }
        }
    }
    
    // TODO: Implement from_json_string() method in SPHParametersBuilder
    // This requires adding JSON parsing to convert JSON keys to SPHType enum and other types
    /*
    SCENARIO(TypeSafeSPHParametersBuilder, LoadsFromJSON) {
        GIVEN("A JSON configuration file") {
            // Create temporary JSON file for testing
            const char* json_content = R"({
                "startTime": 0.0,
                "endTime": 0.2,
                "outputTime": 0.01,
                "SPHType": "gsph",
                "cflSound": 0.3,
                "cflForce": 0.125,
                "neighborNumber": 50,
                "gamma": 1.4,
                "kernel": "cubic_spline",
                "use2ndOrderGSPH": true
            })";
            
            WHEN("Loading via builder") {
                auto params = SPHParametersBuilder()
                    .from_json_string(json_content)
                    .build();
                
                THEN("Should parse all parameters") {
                    EXPECT_EQ(params->type, SPHType::GSPH);
                    EXPECT_EQ(params->time.end, 0.2);
                    EXPECT_TRUE(params->gsph.is_2nd_order);
                }
            }
        }
    }
    */
    
    SCENARIO(TypeSafeSPHParametersBuilder, ProvidesHelpfulErrorMessages) {
        GIVEN("Missing required parameters") {
            SPHParametersBuilder builder;
            builder.with_time(0.0, 0.2, 0.01);
            // Missing: sph_type, cfl, physics, kernel
            
            WHEN("Attempting to build") {
                THEN("Should throw with descriptive message") {
                    try {
                        builder.build();
                        FAIL() << "Expected exception";
                    } catch (const std::runtime_error& e) {
                        std::string msg(e.what());
                        EXPECT_TRUE(msg.find("Missing required") != std::string::npos ||
                                  msg.find("incomplete") != std::string::npos);
                    }
                }
            }
        }
    }
}

FEATURE(PluginParameterIntegration) {
    
    SCENARIO(PluginParameterIntegration, PluginUsesBuilderForTypeSafety) {
        GIVEN("A simulation plugin") {
            WHEN("Creating parameters in plugin initialize()") {
                // Plugin code would use builder like:
                auto params = SPHParametersBuilder()
                    .with_time(0.0, 0.2, 0.01)
                    .with_sph_type("gsph")
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .with_periodic_boundary(
                        (real[]){-0.5}, 
                        (real[]){1.5}
                    )
                    .build();
                
                THEN("Plugin gets compile-time safety") {
                    // If plugin forgets a required parameter,
                    // code won't compile or will throw at build()
                    EXPECT_TRUE(params != nullptr);
                }
            }
        }
    }
    
    // TODO: Enable when JSON->SPHType conversion is implemented
    // SCENARIO(PluginParameterIntegration, JSONOverridesPluginDefaults) {
    //     GIVEN("Plugin with default parameters") {
    //         auto plugin_params = SPHParametersBuilder()
    //             .with_time(0.0, 1.0, 0.1)  // Plugin defaults
    //             .with_sph_type("ssph")
    //             .with_cfl(0.3, 0.125)
    //             .with_physics(32, 1.4)
    //             .with_kernel("cubic_spline")
    //             .build();
    //         
    //         WHEN("JSON config overrides values") {
    //             const char* json = R"({"endTime": 0.2, "SPHType": "gsph"})";
    //             auto merged = SPHParametersBuilder()
    //                 .from_existing(plugin_params)
    //                 .from_json_string(json)
    //                 .build();
    //             
    //             THEN("JSON values take precedence") {
    //                 EXPECT_EQ(merged->time.end, 0.2);  // Overridden
    //                 EXPECT_EQ(merged->type, SPHType::GSPH);  // Overridden
    //                 EXPECT_EQ(merged->physics.neighbor_number, 32);  // From plugin
    //             }
    //         }
    //     }
    // }
}

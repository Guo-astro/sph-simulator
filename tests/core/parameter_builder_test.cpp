// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../bdd_helpers.hpp"
#include "parameters.hpp"
#include "core/sph_algorithm_registry.hpp"
#include "core/sph_parameters_builder_base.hpp"
#include "core/ssph_parameters_builder.hpp"
#include "core/disph_parameters_builder.hpp"
#include "core/gsph_parameters_builder.hpp"
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

FEATURE(TypeSafeAlgorithmParametersBuilder) {
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, GSPH_BuildsWithoutViscosity) {
        GIVEN("A base parameter builder") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Transitioning to GSPH and building") {
                auto params = base.as_gsph().build();
                
                THEN("Parameters should be correctly set") {
                    EXPECT_EQ(params->time.start, 0.0);
                    EXPECT_EQ(params->time.end, 0.2);
                    EXPECT_EQ(params->cfl.sound, 0.3);
                    EXPECT_EQ(params->physics.neighbor_number, 50);
                    EXPECT_EQ(params->type, SPHType::GSPH);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, SSPH_RequiresViscosity) {
        GIVEN("SSPH builder without viscosity") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Attempting to build without setting viscosity") {
                auto ssph = base.as_ssph();
                
                THEN("Should throw exception") {
                    EXPECT_THROW(ssph.build(), std::runtime_error);
                }
            }
        }
        
        GIVEN("SSPH builder with viscosity") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Setting artificial viscosity and building") {
                auto params = base.as_ssph()
                    .with_artificial_viscosity(1.0)
                    .build();
                
                THEN("Should build successfully") {
                    EXPECT_EQ(params->type, SPHType::SSPH);
                    EXPECT_EQ(params->av.alpha, 1.0);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, DISPH_RequiresViscosity) {
        GIVEN("DISPH builder with viscosity") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Setting viscosity and building") {
                auto params = base.as_disph()
                    .with_artificial_viscosity(1.0, true, false)
                    .build();
                
                THEN("Parameters should be set") {
                    EXPECT_EQ(params->type, SPHType::DISPH);
                    EXPECT_EQ(params->av.alpha, 1.0);
                    EXPECT_TRUE(params->av.use_balsara_switch);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, GSPH_2ndOrderMUSCL) {
        GIVEN("GSPH builder") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(0.3, 0.125)
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Enabling 2nd order MUSCL") {
                auto params = base.as_gsph()
                    .with_2nd_order_muscl(true)
                    .build();
                
                THEN("Should be enabled") {
                    EXPECT_TRUE(params->gsph.is_2nd_order);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, ValidatesBaseParameters) {
        GIVEN("Builder with invalid CFL") {
            auto base = SPHParametersBuilderBase()
                .with_time(0.0, 0.2, 0.01)
                .with_cfl(1.5, 0.125)  // CFL > 1.0
                .with_physics(50, 1.4)
                .with_kernel("cubic_spline");
            
            WHEN("Attempting to transition to algorithm") {
                THEN("Should throw validation error") {
                    EXPECT_THROW(base.as_gsph(), std::runtime_error);
                }
            }
        }
    }
    
    SCENARIO(TypeSafeAlgorithmParametersBuilder, SupportsMethodChaining) {
        GIVEN("A builder") {
            WHEN("Chaining multiple methods") {
                auto params = SPHParametersBuilderBase()
                    .with_time(0.0, 0.2, 0.01)
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .with_gravity(9.81)
                    .with_tree_params(20, 1)
                    .as_ssph()
                    .with_artificial_viscosity(1.0)
                    .with_artificial_conductivity(1.0)
                    .build();
                
                THEN("All parameters should be set") {
                    EXPECT_EQ(params->type, SPHType::SSPH);
                    EXPECT_TRUE(params->gravity.is_valid);
                    EXPECT_EQ(params->gravity.constant, 9.81);
                    EXPECT_TRUE(params->ac.is_valid);
                }
            }
        }
FEATURE(PluginParameterIntegration) {
    
    SCENARIO(PluginParameterIntegration, PluginUsesNewBuilderForTypeSafety) {
        GIVEN("A simulation plugin") {
            WHEN("Creating GSPH parameters") {
                auto params = SPHParametersBuilderBase()
                    .with_time(0.0, 0.2, 0.01)
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .as_gsph()
                    .with_2nd_order_muscl(true)
                    .build();
                
                THEN("Plugin gets compile-time safety") {
                    EXPECT_TRUE(params != nullptr);
                    EXPECT_EQ(params->type, SPHType::GSPH);
                }
            }
            
            WHEN("Creating SSPH parameters") {
                auto params = SPHParametersBuilderBase()
                    .with_time(0.0, 0.2, 0.01)
                    .with_cfl(0.3, 0.125)
                    .with_physics(50, 1.4)
                    .with_kernel("cubic_spline")
                    .as_ssph()
                    .with_artificial_viscosity(1.0)
                    .build();
                
                THEN("Must set artificial viscosity") {
                    EXPECT_EQ(params->type, SPHType::SSPH);
                    EXPECT_EQ(params->av.alpha, 1.0);
                }
            }
        }
    }
}

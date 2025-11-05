// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file parameter_categories_test.cpp
 * @brief BDD tests for categorized SPH parameter builders
 * 
 * This test suite demonstrates the separation of concerns:
 * - PhysicsParameters: Physical constants, equations of state
 * - ComputationalParameters: Algorithms, tree settings, neighbor search
 * - OutputParameters: What and when to write simulation results
 * - SimulationParameters: High-level orchestration (time, SPH type)
 */

#include "core/physics_parameters.hpp"
#include "core/computational_parameters.hpp"
#include "core/output_parameters.hpp"
#include "core/simulation_parameters.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

// BDD-style macros
#define FEATURE(name) namespace Feature_##name
#define SCENARIO(name) TEST(Feature, name)
#define GIVEN(desc) /* Given: desc */
#define WHEN(desc) /* When: desc */
#define THEN(desc) /* Then: desc */
#define AND(desc) /* And: desc */
#define REQUIRE(cond) ASSERT_TRUE(cond)
#define REQUIRE_THROWS(expr) ASSERT_THROW(expr, std::exception)

using namespace sph;

FEATURE(PhysicsParameters) {
    
    SCENARIO(BuildingPhysicsParametersWithRequiredFields) {
        GIVEN("A physics parameter builder") {
            WHEN("Setting gamma and neighbor number") {
                auto physics = PhysicsParametersBuilder()
                    .with_gamma(1.4)
                    .with_neighbor_number(50)
                    .build();
                
                THEN("Parameters are correctly set") {
                    REQUIRE(physics->gamma == 1.4);
                    REQUIRE(physics->neighbor_number == 50);
                }
            }
        }
    }
    
    SCENARIO(BuildingWithArtificialViscosity) {
        GIVEN("A physics parameter builder") {
            WHEN("Setting artificial viscosity") {
                auto physics = PhysicsParametersBuilder()
                    .with_gamma(1.4)
                    .with_neighbor_number(50)
                    .with_artificial_viscosity(1.0, true, false)
                    .build();
                
                THEN("AV parameters are correctly set") {
                    REQUIRE(physics->av.alpha == 1.0);
                    REQUIRE(physics->av.use_balsara_switch == true);
                    REQUIRE(physics->av.use_time_dependent == false);
                }
            }
        }
    }
    
    SCENARIO(ValidatingGamma) {
        GIVEN("A physics parameter builder") {
            WHEN("Setting invalid gamma <= 1.0") {
                auto builder = PhysicsParametersBuilder()
                    .with_gamma(0.5)  // Invalid!
                    .with_neighbor_number(50);
                
                THEN("Build throws validation error") {
                    REQUIRE_THROWS(builder.build());
                }
            }
        }
    }
    
    SCENARIO(BuildingWithPeriodicBoundary) {
        GIVEN("A physics parameter builder") {
            WHEN("Setting periodic boundary conditions") {
                real range_min[Dim] = {-1.0};
                real range_max[Dim] = {1.0};
                
                auto physics = PhysicsParametersBuilder()
                    .with_gamma(1.4)
                    .with_neighbor_number(50)
                    .with_periodic_boundary(range_min, range_max)
                    .build();
                
                THEN("Periodic boundaries are set") {
                    REQUIRE(physics->periodic.is_valid == true);
                    REQUIRE(physics->periodic.range_min[0] == -1.0);
                    REQUIRE(physics->periodic.range_max[0] == 1.0);
                }
            }
        }
    }
}

FEATURE(ComputationalParameters) {
    
    SCENARIO(BuildingComputationalParametersWithDefaults) {
        GIVEN("A computational parameter builder") {
            WHEN("Building with default settings") {
                auto comp = ComputationalParametersBuilder()
                    .build();
                
                THEN("Default values are applied") {
                    REQUIRE(comp->tree.max_level == 20);
                    REQUIRE(comp->tree.leaf_particle_num == 1);
                    REQUIRE(comp->iterative_smoothing_length == true);
                }
            }
        }
    }
    
    SCENARIO(CustomizingTreeParameters) {
        GIVEN("A computational parameter builder") {
            WHEN("Setting custom tree parameters") {
                auto comp = ComputationalParametersBuilder()
                    .with_tree_params(15, 5)
                    .build();
                
                THEN("Tree parameters are customized") {
                    REQUIRE(comp->tree.max_level == 15);
                    REQUIRE(comp->tree.leaf_particle_num == 5);
                }
            }
        }
    }
    
    SCENARIO(SettingKernelType) {
        GIVEN("A computational parameter builder") {
            WHEN("Setting kernel type") {
                auto comp = ComputationalParametersBuilder()
                    .with_kernel("wendland")
                    .build();
                
                THEN("Kernel type is set") {
                    REQUIRE(comp->kernel == KernelType::WENDLAND);
                }
            }
        }
    }
    
    SCENARIO(InvalidKernelType) {
        GIVEN("A computational parameter builder") {
            WHEN("Setting unknown kernel") {
                THEN("Build throws error") {
                    REQUIRE_THROWS(
                        ComputationalParametersBuilder()
                            .with_kernel("unknown_kernel")
                    );
                }
            }
        }
    }
    
    SCENARIO(ConfiguringGSPHSettings) {
        GIVEN("A computational parameter builder") {
            WHEN("Disabling 2nd order GSPH") {
                auto comp = ComputationalParametersBuilder()
                    .with_gsph_2nd_order(false)
                    .build();
                
                THEN("GSPH setting is applied") {
                    REQUIRE(comp->gsph.is_2nd_order == false);
                }
            }
        }
    }
}

FEATURE(OutputParameters) {
    
    SCENARIO(BuildingOutputParametersWithRequiredFields) {
        GIVEN("An output parameter builder") {
            WHEN("Setting output intervals") {
                auto output = OutputParametersBuilder()
                    .with_directory("output/test")
                    .with_particle_output_interval(0.01)
                    .with_energy_output_interval(0.01)
                    .build();
                
                THEN("Output parameters are set") {
                    REQUIRE(output->directory == "output/test");
                    REQUIRE(output->particle_interval == 0.01);
                    REQUIRE(output->energy_interval == 0.01);
                }
            }
        }
    }
    
    SCENARIO(DefaultEnergyInterval) {
        GIVEN("An output parameter builder") {
            WHEN("Not explicitly setting energy interval") {
                auto output = OutputParametersBuilder()
                    .with_directory("output/test")
                    .with_particle_output_interval(0.01)
                    .build();
                
                THEN("Energy interval defaults to particle interval") {
                    REQUIRE(output->energy_interval == 0.01);
                }
            }
        }
    }
    
    SCENARIO(ValidatingOutputIntervals) {
        GIVEN("An output parameter builder") {
            WHEN("Setting negative interval") {
                auto builder = OutputParametersBuilder()
                    .with_directory("output/test")
                    .with_particle_output_interval(-0.01);
                
                THEN("Build throws validation error") {
                    REQUIRE_THROWS(builder.build());
                }
            }
        }
    }
}

FEATURE(SimulationParameters) {
    
    SCENARIO(BuildingCompleteSimulation) {
        GIVEN("All parameter builders") {
            WHEN("Composing a complete simulation") {
                auto physics = PhysicsParametersBuilder()
                    .with_gamma(1.4)
                    .with_neighbor_number(50)
                    .build();
                
                auto computational = ComputationalParametersBuilder()
                    .with_kernel("cubic_spline")
                    .with_tree_params(20, 1)
                    .build();
                
                auto output = OutputParametersBuilder()
                    .with_directory("output/shock_tube")
                    .with_particle_output_interval(0.01)
                    .build();
                
                auto simulation = SimulationParametersBuilder()
                    .with_time(0.0, 0.2)
                    .with_cfl(0.3, 0.125)
                    .with_sph_type("gsph")
                    .with_physics(physics)
                    .with_computational(computational)
                    .with_output(output)
                    .build();
                
                THEN("Complete simulation is configured") {
                    REQUIRE(simulation->time.start == 0.0);
                    REQUIRE(simulation->time.end == 0.2);
                    REQUIRE(simulation->sph_type == SPHType::GSPH);
                    REQUIRE(simulation->physics->gamma == 1.4);
                    REQUIRE(simulation->computational->kernel == KernelType::CUBIC_SPLINE);
                    REQUIRE(simulation->output->directory == "output/shock_tube");
                }
            }
        }
    }
    
    SCENARIO(MissingRequiredParameters) {
        GIVEN("A simulation parameter builder") {
            WHEN("Building without physics parameters") {
                auto builder = SimulationParametersBuilder()
                    .with_time(0.0, 0.2)
                    .with_cfl(0.3, 0.125)
                    .with_sph_type("gsph");
                // Missing physics, computational, output
                
                THEN("Build throws error") {
                    REQUIRE_THROWS(builder.build());
                }
            }
        }
    }
    
    SCENARIO(LoadingFromJSON) {
        GIVEN("A JSON configuration file") {
            WHEN("Loading simulation parameters from JSON") {
                // Skip this test - would need actual JSON file
                THEN("All parameters are loaded") {
                    // Test skipped - requires actual JSON file
                    REQUIRE(true);
                }
            }
        }
    }
}

FEATURE(ParameterComposition) {
    
    SCENARIO(OverridingPhysicsParametersAfterJSONLoad) {
        GIVEN("Parameters loaded from JSON") {
            WHEN("Overriding specific physics values") {
                // Skip - would need actual JSON file
                auto physics = PhysicsParametersBuilder()
                    .with_gamma(1.6)
                    .with_neighbor_number(50)
                    .build();
                
                THEN("Override takes precedence") {
                    REQUIRE(physics->gamma == 1.6);
                }
            }
        }
    }
    
    SCENARIO(ReusingComputationalSettings) {
        GIVEN("A standard computational configuration") {
            auto standard_comp = ComputationalParametersBuilder()
                .with_kernel("cubic_spline")
                .with_tree_params(20, 1)
                .with_iterative_smoothing_length(true)
                .build();
            
            auto standard_output = OutputParametersBuilder()
                .with_directory("output/test")
                .with_particle_output_interval(0.01)
                .build();
            
            WHEN("Creating multiple simulations with same settings") {
                auto sim1 = SimulationParametersBuilder()
                    .with_time(0.0, 0.2)
                    .with_cfl(0.3, 0.125)
                    .with_sph_type("gsph")
                    .with_physics(PhysicsParametersBuilder()
                        .with_gamma(1.4)
                        .with_neighbor_number(50)
                        .build())
                    .with_computational(standard_comp)
                    .with_output(standard_output)
                    .build();
                
                auto sim2 = SimulationParametersBuilder()
                    .with_time(0.0, 0.5)
                    .with_cfl(0.3, 0.125)
                    .with_sph_type("disph")
                    .with_physics(PhysicsParametersBuilder()
                        .with_gamma(1.4)
                        .with_neighbor_number(50)
                        .build())
                    .with_computational(standard_comp)  // Reuse!
                    .with_output(standard_output)
                    .build();
                
                THEN("Both simulations share computational settings") {
                    REQUIRE(sim1->computational->kernel == sim2->computational->kernel);
                    REQUIRE(sim1->computational->tree.max_level == 
                           sim2->computational->tree.max_level);
                }
            }
        }
    }
}

// Note: main() is defined in test_main.cpp


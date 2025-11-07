/**
 * @file gravity_variant_test.cpp
 * @brief BDD tests for type-safe gravity variant (std::variant)
 * 
 * Verifies that:
 * 1. std::variant eliminates runtime boolean checks
 * 2. Pattern matching with std::visit is compile-time safe
 * 3. Invalid access throws std::bad_variant_access
 * 4. Type discrimination works correctly
 */

#include <gtest/gtest.h>
#include "parameters.hpp"
#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include <variant>

using namespace sph;

class GravityVariantTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ==================== BDD TESTS ====================

TEST_F(GravityVariantTest, GivenNoGravity_WhenCheckingType_ThenHoldsNoGravity) {
    // GIVEN: Parameters with no gravity (default)
    GIVEN("Parameters constructed without gravity") {
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // WHEN: Checking gravity type
        WHEN("Checking if gravity is enabled") {
            const bool has_grav = params->has_gravity();
            const bool is_no_gravity = std::holds_alternative<SPHParameters::NoGravity>(
                params->get_gravity()
            );
            
            // THEN: Should hold NoGravity variant
            THEN("Gravity is disabled") {
                EXPECT_FALSE(has_grav);
                EXPECT_TRUE(is_no_gravity);
            }
        }
    }
}

TEST_F(GravityVariantTest, GivenNewtonianGravity_WhenCheckingType_ThenHoldsNewtonianGravity) {
    // GIVEN: Parameters with Newtonian gravity
    GIVEN("Parameters with Newtonian gravity enabled") {
        constexpr real G = 6.674e-8;
        constexpr real theta = 0.5;
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .with_gravity(G, theta)  // Enable gravity
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // WHEN: Checking gravity type
        WHEN("Checking gravity configuration") {
            const bool has_grav = params->has_gravity();
            const bool is_newtonian = std::holds_alternative<SPHParameters::NewtonianGravity>(
                params->get_gravity()
            );
            
            // THEN: Should hold NewtonianGravity variant
            THEN("Newtonian gravity is enabled with correct parameters") {
                EXPECT_TRUE(has_grav);
                EXPECT_TRUE(is_newtonian);
                
                const auto& newtonian = params->get_newtonian_gravity();
                EXPECT_DOUBLE_EQ(newtonian.constant, G);
                EXPECT_DOUBLE_EQ(newtonian.theta, theta);
            }
        }
    }
}

TEST_F(GravityVariantTest, GivenNoGravity_WhenAccessingNewtonian_ThenThrowsBadVariantAccess) {
    // GIVEN: Parameters without gravity
    GIVEN("Parameters with no gravity") {
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // WHEN: Attempting to access Newtonian gravity config
        WHEN("Trying to get Newtonian gravity from NoGravity variant") {
            // THEN: Should throw std::bad_variant_access
            THEN("Throws bad_variant_access exception") {
                EXPECT_THROW(
                    params->get_newtonian_gravity(),
                    std::bad_variant_access
                );
            }
        }
    }
}

TEST_F(GravityVariantTest, GivenNewtonianGravity_WhenVisitingWithPattern_ThenCallsNewtonianBranch) {
    // GIVEN: Parameters with Newtonian gravity
    GIVEN("Parameters with Newtonian gravity") {
        constexpr real G = 1.0;
        constexpr real theta = 0.5;
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .with_gravity(G, theta)
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // WHEN: Using std::visit for pattern matching
        WHEN("Pattern matching with std::visit") {
            bool called_newtonian = false;
            bool called_no_gravity = false;
            real extracted_g = 0.0;
            real extracted_theta = 0.0;
            
            params->visit_gravity([&](auto&& g) {
                using T = std::decay_t<decltype(g)>;
                if constexpr (std::is_same_v<T, SPHParameters::NewtonianGravity>) {
                    called_newtonian = true;
                    extracted_g = g.constant;
                    extracted_theta = g.theta;
                } else if constexpr (std::is_same_v<T, SPHParameters::NoGravity>) {
                    called_no_gravity = true;
                }
            });
            
            // THEN: Newtonian branch called, correct values extracted
            THEN("Newtonian branch executed with correct values") {
                EXPECT_TRUE(called_newtonian);
                EXPECT_FALSE(called_no_gravity);
                EXPECT_DOUBLE_EQ(extracted_g, G);
                EXPECT_DOUBLE_EQ(extracted_theta, theta);
            }
        }
    }
}

TEST_F(GravityVariantTest, GivenNoGravity_WhenVisitingWithPattern_ThenCallsNoGravityBranch) {
    // GIVEN: Parameters without gravity
    GIVEN("Parameters with no gravity") {
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline")
            .as_ssph()
            .with_artificial_viscosity(1.0)
            .build();
        
        // WHEN: Using std::visit for pattern matching
        WHEN("Pattern matching with std::visit") {
            bool called_newtonian = false;
            bool called_no_gravity = false;
            
            params->visit_gravity([&](auto&& g) {
                using T = std::decay_t<decltype(g)>;
                if constexpr (std::is_same_v<T, SPHParameters::NewtonianGravity>) {
                    called_newtonian = true;
                } else if constexpr (std::is_same_v<T, SPHParameters::NoGravity>) {
                    called_no_gravity = true;
                }
            });
            
            // THEN: NoGravity branch called
            THEN("NoGravity branch executed") {
                EXPECT_FALSE(called_newtonian);
                EXPECT_TRUE(called_no_gravity);
            }
        }
    }
}

TEST_F(GravityVariantTest, GivenVariantType_WhenCompiling_ThenNoRuntimeBooleanChecks) {
    // GIVEN: Type-safe variant design
    GIVEN("std::variant-based gravity configuration") {
        // WHEN: Checking type system guarantees
        WHEN("Verifying compile-time type safety") {
            // THEN: variant enforces type safety at compile time
            THEN("No is_valid boolean runtime checks needed") {
                // Compile-time checks
                static_assert(
                    std::variant_size_v<SPHParameters::GravityVariant> == 3,
                    "GravityVariant should have exactly 3 alternatives"
                );
                
                static_assert(
                    std::is_same_v<
                        std::variant_alternative_t<0, SPHParameters::GravityVariant>,
                        SPHParameters::NoGravity
                    >,
                    "First alternative should be NoGravity"
                );
                
                static_assert(
                    std::is_same_v<
                        std::variant_alternative_t<1, SPHParameters::GravityVariant>,
                        SPHParameters::NewtonianGravity
                    >,
                    "Second alternative should be NewtonianGravity"
                );
                
                // Success if compiles
                SUCCEED();
            }
        }
    }
}

// ==================== INTEGRATION TEST ====================

TEST_F(GravityVariantTest, GivenEvrardScenario_WhenUsingGravity_ThenCorrectTypeSelected) {
    // GIVEN: Evrard collapse scenario with gravity
    GIVEN("Evrard gravitational collapse parameters") {
        constexpr real G = 1.0;
        constexpr real theta = 0.5;
        constexpr real gamma = 5.0 / 3.0;
        
        auto params = SPHParametersBuilderBase()
            .with_time(0.0, 3.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, gamma)
            .with_kernel("cubic_spline")
            .with_gravity(G, theta)  // CRITICAL for Evrard
            .with_tree_params(20, 1)
            .as_ssph()
            .with_artificial_viscosity(1.0, true)
            .build();
        
        // WHEN: Checking gravity configuration
        WHEN("Verifying gravity is correctly configured") {
            // THEN: Should have Newtonian gravity with correct parameters
            THEN("Newtonian gravity enabled for self-gravitating collapse") {
                EXPECT_TRUE(params->has_gravity());
                EXPECT_TRUE(std::holds_alternative<SPHParameters::NewtonianGravity>(
                    params->get_gravity()
                ));
                
                const auto& grav = params->get_newtonian_gravity();
                EXPECT_DOUBLE_EQ(grav.constant, G);
                EXPECT_DOUBLE_EQ(grav.theta, theta);
            }
        }
    }
}

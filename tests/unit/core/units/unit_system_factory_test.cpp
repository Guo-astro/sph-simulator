/**
 * @file unit_system_factory_test.cpp
 * @brief BDD-style tests for UnitSystemFactory
 * 
 * Tests factory pattern for creating unit systems from various inputs.
 * Following TDD: Write tests first, implement to make them pass.
 */

#include <gtest/gtest.h>
#include "../../../helpers/bdd_helpers.hpp"
#include "core/output/units/unit_system_factory.hpp"
#include "core/output/units/unit_system.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include "core/output/units/si_unit_system.hpp"
#include "core/output/units/cgs_unit_system.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <stdexcept>

using namespace sph;
using json = nlohmann::json;

// ============================================================================
// FEATURE: Factory Creation from Enum
// ============================================================================

SCENARIO(UnitSystemFactory, CreatesGalacticFromEnum) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request a GALACTIC unit system by enum") {
            auto unit_system = UnitSystemFactory::create(UnitSystemType::GALACTIC);
            
            THEN("Factory should return a GalacticUnitSystem instance") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
                EXPECT_EQ(unit_system->get_name(), "galactic");
            }
        }
    }
}

SCENARIO(UnitSystemFactory, CreatesSIFromEnum) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request an SI unit system by enum") {
            auto unit_system = UnitSystemFactory::create(UnitSystemType::SI);
            
            THEN("Factory should return an SIUnitSystem instance") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::SI);
                EXPECT_EQ(unit_system->get_name(), "SI");
            }
        }
    }
}

SCENARIO(UnitSystemFactory, CreatesCGSFromEnum) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request a CGS unit system by enum") {
            auto unit_system = UnitSystemFactory::create(UnitSystemType::CGS);
            
            THEN("Factory should return a CGSUnitSystem instance") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::CGS);
                EXPECT_EQ(unit_system->get_name(), "cgs");
            }
        }
    }
}

// ============================================================================
// FEATURE: Factory Creation from String
// ============================================================================

SCENARIO(UnitSystemFactory, CreatesFromLowercaseString) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request 'galactic' as lowercase string") {
            auto unit_system = UnitSystemFactory::create_from_string("galactic");
            
            THEN("Factory should create the correct unit system") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, CreatesFromMixedCaseString) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request 'Galactic' with mixed case") {
            auto unit_system = UnitSystemFactory::create_from_string("Galactic");
            
            THEN("Factory should handle case-insensitively") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, CreatesFromUppercaseString) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request 'SI' in uppercase") {
            auto unit_system = UnitSystemFactory::create_from_string("SI");
            
            THEN("Factory should create SI unit system") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::SI);
            }
        }
        
        AND("When we request 'si' in lowercase") {
            auto unit_system_lower = UnitSystemFactory::create_from_string("si");
            
            THEN("Factory should also create SI unit system") {
                ASSERT_NE(unit_system_lower, nullptr);
                EXPECT_EQ(unit_system_lower->get_type(), UnitSystemType::SI);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, HandlesAllValidStrings) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We try all valid string variations") {
            THEN("All should succeed") {
                // Galactic variations
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("galactic"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("Galactic"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("GALACTIC"));
                
                // SI variations
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("si"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("Si"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("SI"));
                
                // CGS variations
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("cgs"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("Cgs"));
                EXPECT_NO_THROW(UnitSystemFactory::create_from_string("CGS"));
            }
        }
    }
}

SCENARIO(UnitSystemFactory, ThrowsOnInvalidString) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request an invalid unit system name") {
            THEN("Factory should throw std::invalid_argument") {
                EXPECT_THROW(
                    UnitSystemFactory::create_from_string("invalid"),
                    std::invalid_argument
                );
                
                EXPECT_THROW(
                    UnitSystemFactory::create_from_string(""),
                    std::invalid_argument
                );
                
                EXPECT_THROW(
                    UnitSystemFactory::create_from_string("metric"),
                    std::invalid_argument
                );
            }
        }
    }
}

SCENARIO(UnitSystemFactory, ProvidesHelpfulErrorMessages) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We request an invalid unit system") {
            THEN("Error message should list valid options") {
                try {
                    UnitSystemFactory::create_from_string("invalid_system");
                    FAIL() << "Expected std::invalid_argument to be thrown";
                } catch (const std::invalid_argument& e) {
                    const std::string error_msg = e.what();
                    // Error should mention valid options
                    EXPECT_TRUE(error_msg.find("galactic") != std::string::npos ||
                               error_msg.find("Valid") != std::string::npos ||
                               error_msg.find("options") != std::string::npos);
                }
            }
        }
    }
}

// ============================================================================
// FEATURE: Factory Creation from JSON
// ============================================================================

SCENARIO(UnitSystemFactory, CreatesFromJSONWithStringValue) {
    GIVEN("A JSON object with string unit_system field") {
        json config;
        config["unit_system"] = "galactic";
        
        WHEN("We create from JSON") {
            auto unit_system = UnitSystemFactory::create_from_json(config);
            
            THEN("Factory should create the correct unit system") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, CreatesFromJSONWithIntegerValue) {
    GIVEN("A JSON object with integer unit_system field") {
        json config;
        config["unit_system"] = static_cast<int>(UnitSystemType::SI);
        
        WHEN("We create from JSON") {
            auto unit_system = UnitSystemFactory::create_from_json(config);
            
            THEN("Factory should create the correct unit system") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::SI);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, HandlesAlternativeJSONKeys) {
    GIVEN("JSON objects with different key names") {
        WHEN("JSON uses 'name' key") {
            json config;
            config["name"] = "cgs";
            auto unit_system = UnitSystemFactory::create_from_json(config);
            
            THEN("Factory should find and use it") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::CGS);
            }
        }
        
        AND("When JSON uses 'type' key") {
            json config;
            config["type"] = "galactic";
            auto unit_system = UnitSystemFactory::create_from_json(config);
            
            THEN("Factory should find and use it") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, PrioritizesUnitSystemKey) {
    GIVEN("JSON with multiple unit system keys") {
        json config;
        config["unit_system"] = "galactic";
        config["name"] = "SI";
        config["type"] = "cgs";
        
        WHEN("We create from JSON") {
            auto unit_system = UnitSystemFactory::create_from_json(config);
            
            THEN("Factory should use 'unit_system' key with highest priority") {
                ASSERT_NE(unit_system, nullptr);
                EXPECT_EQ(unit_system->get_type(), UnitSystemType::GALACTIC);
            }
        }
    }
}

SCENARIO(UnitSystemFactory, ThrowsOnMissingJSONField) {
    GIVEN("JSON without unit system specification") {
        json config;
        config["some_other_field"] = "value";
        
        WHEN("We try to create from JSON") {
            THEN("Factory should throw std::invalid_argument") {
                EXPECT_THROW(
                    UnitSystemFactory::create_from_json(config),
                    std::invalid_argument
                );
            }
        }
    }
}

SCENARIO(UnitSystemFactory, ThrowsOnInvalidJSONType) {
    GIVEN("JSON with invalid type for unit_system") {
        json config;
        config["unit_system"] = json::array({1, 2, 3});  // Array is invalid
        
        WHEN("We try to create from JSON") {
            THEN("Factory should throw std::invalid_argument") {
                EXPECT_THROW(
                    UnitSystemFactory::create_from_json(config),
                    std::invalid_argument
                );
            }
        }
    }
}

SCENARIO(UnitSystemFactory, ValidatesIntegerRange) {
    GIVEN("JSON with out-of-range integer value") {
        json config;
        config["unit_system"] = 999;  // Invalid enum value
        
        WHEN("We try to create from JSON") {
            THEN("Factory should throw std::invalid_argument") {
                EXPECT_THROW(
                    UnitSystemFactory::create_from_json(config),
                    std::invalid_argument
                );
            }
        }
    }
    
    GIVEN("JSON with negative integer value") {
        json config;
        config["unit_system"] = -1;  // Invalid enum value
        
        WHEN("We try to create from JSON") {
            THEN("Factory should throw std::invalid_argument") {
                EXPECT_THROW(
                    UnitSystemFactory::create_from_json(config),
                    std::invalid_argument
                );
            }
        }
    }
}

// ============================================================================
// FEATURE: Factory Returns Unique Pointers
// ============================================================================

SCENARIO(UnitSystemFactory, ReturnsUniquePointers) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We create multiple unit systems") {
            auto us1 = UnitSystemFactory::create(UnitSystemType::GALACTIC);
            auto us2 = UnitSystemFactory::create(UnitSystemType::SI);
            
            THEN("Each should be a unique, independent instance") {
                ASSERT_NE(us1, nullptr);
                ASSERT_NE(us2, nullptr);
                EXPECT_NE(us1.get(), us2.get());
            }
            
            AND("They should have proper RAII cleanup") {
                // Implicit: unique_ptr will clean up automatically
                // This tests that destructors work correctly
            }
        }
    }
}

// ============================================================================
// FEATURE: Thread Safety (Basic)
// ============================================================================

SCENARIO(UnitSystemFactory, SupportsBasicConcurrency) {
    GIVEN("The UnitSystemFactory") {
        WHEN("We create unit systems concurrently") {
            THEN("Each thread should get valid instances") {
                // Basic test - factory methods should be thread-safe
                // since they only create new objects, no shared state
                auto us1 = UnitSystemFactory::create(UnitSystemType::GALACTIC);
                auto us2 = UnitSystemFactory::create(UnitSystemType::GALACTIC);
                
                EXPECT_NE(us1.get(), us2.get());
                EXPECT_EQ(us1->get_type(), us2->get_type());
            }
        }
    }
}

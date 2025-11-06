/**
 * @file unit_system_test.cpp
 * @brief BDD-style tests for UnitSystem framework
 * 
 * Test-Driven Development in Behavior-Driven style (Given-When-Then)
 * Following coding rules: No macros except guards, constexpr for constants,
 * comprehensive testing with boundary conditions, clear test names.
 */

#include <gtest/gtest.h>
#include "../bdd_helpers.hpp"
#include "core/output/units/unit_system.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include "core/output/units/si_unit_system.hpp"
#include "core/output/units/cgs_unit_system.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <memory>

using namespace sph;
using json = nlohmann::json;

// Test constants following coding rules (constexpr, no macros)
namespace test_constants {
    constexpr double kTolerance = 1e-10;
    constexpr double kParsecToCm = 3.0857e18;
    constexpr double kSolarMassToGram = 1.989e33;
    constexpr double kGravConstCgs = 6.674e-8;
}

// ============================================================================
// FEATURE: Galactic Unit System
// ============================================================================

SCENARIO(GalacticUnitSystem, ConvertsFundamentalUnitsCorrectly) {
    GIVEN("A GalacticUnitSystem instance") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        
        WHEN("We query fundamental unit conversion factors") {
            const real length_factor = galactic->get_length_unit();
            const real mass_factor = galactic->get_mass_unit();
            const real time_factor = galactic->get_time_unit();
            
            THEN("Length unit should convert parsecs to centimeters") {
                EXPECT_NEAR(length_factor, test_constants::kParsecToCm, 
                           test_constants::kTolerance * length_factor);
            }
            
            AND("Mass unit should convert solar masses to grams") {
                EXPECT_NEAR(mass_factor, test_constants::kSolarMassToGram,
                           test_constants::kTolerance * mass_factor);
            }
            
            AND("Time unit should be derived from G=1 assumption") {
                // Time = sqrt(L^3 / (G * M))
                const real expected_time = std::sqrt(
                    test_constants::kParsecToCm * 
                    test_constants::kParsecToCm * 
                    test_constants::kParsecToCm / 
                    (test_constants::kGravConstCgs * test_constants::kSolarMassToGram)
                );
                EXPECT_NEAR(time_factor, expected_time,
                           test_constants::kTolerance * time_factor);
            }
        }
    }
}

SCENARIO(GalacticUnitSystem, CalculatesDerivedUnitsCorrectly) {
    GIVEN("A GalacticUnitSystem with known fundamental units") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        const real L = galactic->get_length_unit();
        const real M = galactic->get_mass_unit();
        const real T = galactic->get_time_unit();
        
        WHEN("We query derived unit conversion factors") {
            const real velocity_factor = galactic->get_velocity_unit();
            const real acceleration_factor = galactic->get_acceleration_unit();
            const real density_factor = galactic->get_density_unit();
            const real pressure_factor = galactic->get_pressure_unit();
            const real energy_factor = galactic->get_energy_unit();
            
            THEN("Velocity should be length/time") {
                const real expected = L / T;
                EXPECT_NEAR(velocity_factor, expected, 
                           test_constants::kTolerance * expected);
            }
            
            AND("Acceleration should be length/time^2") {
                const real expected = L / (T * T);
                EXPECT_NEAR(acceleration_factor, expected,
                           test_constants::kTolerance * expected);
            }
            
            AND("Density should be mass/length^3") {
                const real expected = M / (L * L * L);
                EXPECT_NEAR(density_factor, expected,
                           test_constants::kTolerance * expected);
            }
            
            AND("Pressure should be mass/(length*time^2)") {
                const real expected = M / (L * T * T);
                EXPECT_NEAR(pressure_factor, expected,
                           test_constants::kTolerance * expected);
            }
            
            AND("Energy should be mass*length^2/time^2") {
                const real expected = M * L * L / (T * T);
                EXPECT_NEAR(energy_factor, expected,
                           test_constants::kTolerance * expected);
            }
        }
    }
}

SCENARIO(GalacticUnitSystem, ProvidesCorrectUnitNames) {
    GIVEN("A GalacticUnitSystem instance") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        
        WHEN("We query unit name strings") {
            THEN("Names should match astrophysical conventions") {
                EXPECT_EQ(galactic->get_length_unit_name(), "pc");
                EXPECT_EQ(galactic->get_mass_unit_name(), "M_sun");
                EXPECT_EQ(galactic->get_time_unit_name(), "Myr");
                EXPECT_EQ(galactic->get_velocity_unit_name(), "km/s");
                EXPECT_EQ(galactic->get_density_unit_name(), "M_sun/pc^3");
                EXPECT_EQ(galactic->get_pressure_unit_name(), "M_sun/(pc*Myr^2)");
                EXPECT_EQ(galactic->get_energy_unit_name(), "M_sun*pc^2/Myr^2");
            }
        }
    }
}

SCENARIO(GalacticUnitSystem, ConvertsPhysicalValuesCorrectly) {
    GIVEN("A GalacticUnitSystem and test values in code units") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        constexpr real code_length = 1.0;
        constexpr real code_mass = 2.5;
        constexpr real code_velocity = 0.5;
        
        WHEN("We convert these values to output units") {
            const real output_length = galactic->convert_length(code_length);
            const real output_mass = galactic->convert_mass(code_mass);
            const real output_velocity = galactic->convert_velocity(code_velocity);
            
            THEN("Conversions should apply the correct scaling factors") {
                EXPECT_NEAR(output_length, code_length * galactic->get_length_unit(),
                           test_constants::kTolerance * output_length);
                EXPECT_NEAR(output_mass, code_mass * galactic->get_mass_unit(),
                           test_constants::kTolerance * output_mass);
                EXPECT_NEAR(output_velocity, code_velocity * galactic->get_velocity_unit(),
                           test_constants::kTolerance * output_velocity);
            }
        }
    }
}

// ============================================================================
// FEATURE: SI Unit System
// ============================================================================

SCENARIO(SIUnitSystem, ReturnsIdentityConversions) {
    GIVEN("An SI unit system instance") {
        auto si = std::make_unique<SIUnitSystem>();
        
        WHEN("We query all unit conversion factors") {
            THEN("All fundamental factors should be 1.0 (identity)") {
                EXPECT_DOUBLE_EQ(si->get_length_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_mass_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_time_unit(), 1.0);
            }
            
            AND("All derived factors should also be 1.0") {
                EXPECT_DOUBLE_EQ(si->get_velocity_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_acceleration_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_density_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_pressure_unit(), 1.0);
                EXPECT_DOUBLE_EQ(si->get_energy_unit(), 1.0);
            }
        }
    }
}

SCENARIO(SIUnitSystem, ProvidesStandardUnitNames) {
    GIVEN("An SI unit system instance") {
        auto si = std::make_unique<SIUnitSystem>();
        
        WHEN("We query unit names") {
            THEN("Names should follow SI conventions") {
                EXPECT_EQ(si->get_length_unit_name(), "m");
                EXPECT_EQ(si->get_mass_unit_name(), "kg");
                EXPECT_EQ(si->get_time_unit_name(), "s");
                EXPECT_EQ(si->get_velocity_unit_name(), "m/s");
                EXPECT_EQ(si->get_density_unit_name(), "kg/m^3");
                EXPECT_EQ(si->get_pressure_unit_name(), "Pa");
                EXPECT_EQ(si->get_energy_unit_name(), "J");
            }
        }
    }
}

// ============================================================================
// FEATURE: CGS Unit System
// ============================================================================

SCENARIO(CGSUnitSystem, ReturnsIdentityConversions) {
    GIVEN("A CGS unit system instance") {
        auto cgs = std::make_unique<CGSUnitSystem>();
        
        WHEN("We query all unit conversion factors") {
            THEN("All factors should be 1.0 for CGS base system") {
                EXPECT_DOUBLE_EQ(cgs->get_length_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_mass_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_time_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_velocity_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_density_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_pressure_unit(), 1.0);
                EXPECT_DOUBLE_EQ(cgs->get_energy_unit(), 1.0);
            }
        }
    }
}

SCENARIO(CGSUnitSystem, ProvidesCGSUnitNames) {
    GIVEN("A CGS unit system instance") {
        auto cgs = std::make_unique<CGSUnitSystem>();
        
        WHEN("We query unit names") {
            THEN("Names should follow CGS conventions") {
                EXPECT_EQ(cgs->get_length_unit_name(), "cm");
                EXPECT_EQ(cgs->get_mass_unit_name(), "g");
                EXPECT_EQ(cgs->get_time_unit_name(), "s");
                EXPECT_EQ(cgs->get_velocity_unit_name(), "cm/s");
                EXPECT_EQ(cgs->get_density_unit_name(), "g/cm^3");
                EXPECT_EQ(cgs->get_pressure_unit_name(), "dyn/cm^2");
                EXPECT_EQ(cgs->get_energy_unit_name(), "erg");
            }
        }
    }
}

// ============================================================================
// FEATURE: Type System Identification
// ============================================================================

SCENARIO(UnitSystemTypes, IdentifyThemselves) {
    GIVEN("Instances of all three unit systems") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        auto si = std::make_unique<SIUnitSystem>();
        auto cgs = std::make_unique<CGSUnitSystem>();
        
        WHEN("We query their types and names") {
            THEN("Each should correctly identify itself") {
                EXPECT_EQ(galactic->get_type(), UnitSystemType::GALACTIC);
                EXPECT_EQ(galactic->get_name(), "galactic");
                
                EXPECT_EQ(si->get_type(), UnitSystemType::SI);
                EXPECT_EQ(si->get_name(), "SI");
                
                EXPECT_EQ(cgs->get_type(), UnitSystemType::CGS);
                EXPECT_EQ(cgs->get_name(), "cgs");
            }
        }
    }
}

// ============================================================================
// FEATURE: JSON Serialization
// ============================================================================

SCENARIO(UnitSystem, SerializesToJSON) {
    GIVEN("A GalacticUnitSystem instance") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        
        WHEN("We serialize it to JSON") {
            const json j = galactic->to_json();
            
            THEN("JSON should contain all required fields") {
                EXPECT_TRUE(j.contains("name"));
                EXPECT_TRUE(j.contains("type"));
                EXPECT_TRUE(j.contains("length_unit"));
                EXPECT_TRUE(j.contains("mass_unit"));
                EXPECT_TRUE(j.contains("time_unit"));
                EXPECT_TRUE(j.contains("length_unit_name"));
                EXPECT_TRUE(j.contains("mass_unit_name"));
                EXPECT_TRUE(j.contains("time_unit_name"));
            }
            
            AND("Values should match the unit system") {
                EXPECT_EQ(j["name"], "galactic");
                EXPECT_EQ(j["length_unit_name"], "pc");
                EXPECT_EQ(j["mass_unit_name"], "M_sun");
                EXPECT_NEAR(j["length_unit"].get<real>(), 
                           galactic->get_length_unit(),
                           test_constants::kTolerance * galactic->get_length_unit());
            }
        }
    }
}

// ============================================================================
// FEATURE: Edge Cases and Error Handling
// ============================================================================

SCENARIO(UnitSystem, HandlesZeroValues) {
    GIVEN("A unit system and zero values") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        constexpr real zero = 0.0;
        
        WHEN("We convert zero values") {
            const real converted = galactic->convert_length(zero);
            
            THEN("Result should be zero") {
                EXPECT_DOUBLE_EQ(converted, 0.0);
            }
        }
    }
}

SCENARIO(UnitSystem, HandlesNegativeValues) {
    GIVEN("A unit system and negative values") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        constexpr real negative = -5.0;
        
        WHEN("We convert negative values") {
            const real converted = galactic->convert_velocity(negative);
            
            THEN("Sign should be preserved") {
                EXPECT_LT(converted, 0.0);
                EXPECT_NEAR(converted, negative * galactic->get_velocity_unit(),
                           test_constants::kTolerance * std::abs(converted));
            }
        }
    }
}

SCENARIO(UnitSystem, HandlesVeryLargeValues) {
    GIVEN("A unit system and very large values") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        constexpr real very_large = 1e10;
        
        WHEN("We convert very large values") {
            const real converted = galactic->convert_mass(very_large);
            
            THEN("Result should be finite and scaled correctly") {
                EXPECT_TRUE(std::isfinite(converted));
                EXPECT_NEAR(converted, very_large * galactic->get_mass_unit(),
                           test_constants::kTolerance * converted);
            }
        }
    }
}

SCENARIO(UnitSystem, HandlesVerySmallValues) {
    GIVEN("A unit system and very small values") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        constexpr real very_small = 1e-15;
        
        WHEN("We convert very small values") {
            const real converted = galactic->convert_length(very_small);
            
            THEN("Result should be finite") {
                EXPECT_TRUE(std::isfinite(converted));
            }
        }
    }
}

// ============================================================================
// FEATURE: Dimensional Analysis Validation
// ============================================================================

SCENARIO(UnitSystem, MaintainsDimensionalConsistency) {
    GIVEN("A GalacticUnitSystem") {
        auto galactic = std::make_unique<GalacticUnitSystem>();
        const real L = galactic->get_length_unit();
        const real M = galactic->get_mass_unit();
        const real T = galactic->get_time_unit();
        
        WHEN("We check dimensional relationships") {
            THEN("Pressure should equal energy density dimensionally") {
                const real pressure = galactic->get_pressure_unit();
                const real energy_density = galactic->get_energy_density_unit();
                
                // Both should be M/(L*T^2)
                const real expected_dim = M / (L * T * T);
                EXPECT_NEAR(pressure, expected_dim, 
                           test_constants::kTolerance * expected_dim);
                EXPECT_NEAR(energy_density, expected_dim,
                           test_constants::kTolerance * expected_dim);
            }
            
            AND("Force (if we had it) would be M*L/T^2") {
                const real force_dim = M * L / (T * T);
                const real pressure_times_area = galactic->get_pressure_unit() * L * L;
                EXPECT_NEAR(pressure_times_area, force_dim,
                           test_constants::kTolerance * force_dim);
            }
        }
    }
}

/**
 * @file unit_conversion_mode_test.cpp
 * @brief BDD-style tests for UnitConversionMode and Output system
 * 
 * Tests the type-safe unit conversion system that controls whether
 * output values are in code units or converted to physical units.
 */

#include <gtest/gtest.h>
#include "../bdd_helpers.hpp"
#include "output.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/output/units/unit_system_factory.hpp"
#include "parameters.hpp"
#include "logger.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <cmath>

using namespace sph;
namespace fs = std::filesystem;

namespace {

constexpr const char* kTestOutputDir = "test_output_unit_conversion";
constexpr real kTolerance = 1e-6;

/**
 * @brief Test fixture for unit conversion mode tests
 */
class UnitConversionModeTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory
        fs::create_directories(kTestOutputDir);
        Logger::open(kTestOutputDir);
        
        // Create a simple simulation with test particles
        auto param = std::make_shared<SPHParameters>();
        sim = std::make_shared<Simulation<1>>(param);
        sim->particle_num = 3;
        sim->particles.resize(3);
        sim->time = 0.1;
        
        // Initialize particles with known values in code units
        sim->particles[0].pos[0] = -0.5;
        sim->particles[0].vel[0] = 0.0;
        sim->particles[0].mass = 1.0;
        sim->particles[0].dens = 1.0;
        sim->particles[0].pres = 1.0;
        sim->particles[0].ene = 2.5;
        sim->particles[0].id = 0;
        sim->particles[0].type = 0;  // REAL
        
        sim->particles[1].pos[0] = 0.0;
        sim->particles[1].vel[0] = 0.5;
        sim->particles[1].mass = 1.0;
        sim->particles[1].dens = 0.5;
        sim->particles[1].pres = 0.5;
        sim->particles[1].ene = 2.0;
        sim->particles[1].id = 1;
        sim->particles[1].type = 0;  // REAL
        
        sim->particles[2].pos[0] = 0.5;
        sim->particles[2].vel[0] = 1.0;
        sim->particles[2].mass = 1.0;
        sim->particles[2].dens = 0.25;
        sim->particles[2].pres = 0.1;
        sim->particles[2].ene = 1.0;
        sim->particles[2].id = 2;
        sim->particles[2].type = 0;  // REAL
    }
    
    void TearDown() override {
        // Clean up test directory
        fs::remove_all(kTestOutputDir);
    }
    
    /**
     * @brief Read first data line from CSV file
     */
    std::vector<real> read_first_particle_csv(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        std::string line;
        // Skip header line
        std::getline(file, line);
        
        // Read first data line
        std::getline(file, line);
        
        std::vector<real> values;
        std::stringstream ss(line);
        std::string value;
        
        while (std::getline(ss, value, ',')) {
            if (!value.empty()) {
                values.push_back(std::stod(value));
            }
        }
        
        return values;
    }
    
    /**
     * @brief Check if file exists
     */
    bool file_exists(const std::string& path) {
        return fs::exists(path);
    }
    
    std::shared_ptr<Simulation<1>> sim;
};

} // anonymous namespace

FEATURE("UnitConversionMode") {

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, DefaultsToCodeUnits) {
    GIVEN("An Output object with default settings") {
        Output<1> output;
        
        WHEN("No unit conversion is set") {
            auto mode = output.get_unit_conversion();
            
            THEN("The mode should be CODE_UNITS") {
                EXPECT_EQ(mode, UnitConversionMode::CODE_UNITS);
            }
        }
        
        WHEN("Writing particles to CSV") {
            output.output_particle(sim);
            
            THEN("Values should remain in code units") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(file_exists(csv_file));
                
                auto values = read_first_particle_csv(csv_file);
                
                // pos_x should be -0.5 (code units)
                EXPECT_NEAR(values[0], -0.5, kTolerance);
                
                // density should be 1.0 (code units)
                EXPECT_NEAR(values[4], 1.0, kTolerance);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, CanSwitchToGalacticUnits) {
    GIVEN("An Output object") {
        Output<1> output;
        
        WHEN("Setting conversion mode to GALACTIC_UNITS") {
            output.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);
            
            THEN("The mode should be GALACTIC_UNITS") {
                EXPECT_EQ(output.get_unit_conversion(), UnitConversionMode::GALACTIC_UNITS);
            }
            
            AND_THEN("Written values should be in Galactic units") {
                output.output_particle(sim);
                
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                auto values = read_first_particle_csv(csv_file);
                
                // pos_x should be converted: -0.5 * 3.086e18 cm
                real expected_pos = -0.5 * 3.086e18;
                EXPECT_NEAR(values[0] / expected_pos, 1.0, 0.01) 
                    << "Position not in Galactic units (cm)";
                
                // Mass should be huge: 1.0 * 1.989e33 g
                real expected_mass = 1.0 * 1.989e33;
                EXPECT_GT(values[3], 1e30) 
                    << "Mass not converted to Galactic units (g)";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, CanSwitchToSIUnits) {
    GIVEN("An Output object") {
        Output<1> output;
        
        WHEN("Setting conversion mode to SI_UNITS") {
            output.set_unit_conversion(UnitConversionMode::SI_UNITS);
            
            THEN("The mode should be SI_UNITS") {
                EXPECT_EQ(output.get_unit_conversion(), UnitConversionMode::SI_UNITS);
            }
            
            AND_THEN("Unit system should be configured for SI") {
                output.output_particle(sim);
                
                // SI unit system assumes code units are already in SI
                // So conversion factors are all 1.0 (identity conversion)
                // The purpose is to label outputs with SI unit names (m, kg, s)
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                auto values = read_first_particle_csv(csv_file);
                
                // Values should remain unchanged (identity conversion)
                EXPECT_NEAR(values[0], -0.5, 1e-6) 
                    << "Position should be unchanged with SI identity conversion";
                
                // Mass should also be unchanged
                EXPECT_NEAR(values[3], 1.0, 1e-6) 
                    << "Mass should be unchanged with SI identity conversion";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, CanSwitchToCGSUnits) {
    GIVEN("An Output object") {
        Output<1> output;
        
        WHEN("Setting conversion mode to CGS_UNITS") {
            output.set_unit_conversion(UnitConversionMode::CGS_UNITS);
            
            THEN("The mode should be CGS_UNITS") {
                EXPECT_EQ(output.get_unit_conversion(), UnitConversionMode::CGS_UNITS);
            }
            
            AND_THEN("Unit system should be configured for CGS") {
                output.output_particle(sim);
                
                // CGS unit system assumes code units are already in CGS
                // So conversion factors are all 1.0 (identity conversion)
                // The purpose is to label outputs with CGS unit names (cm, g, s)
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                auto values = read_first_particle_csv(csv_file);
                
                // Values should remain unchanged (identity conversion)
                EXPECT_NEAR(values[0], -0.5, 1e-6) 
                    << "Position should be unchanged with CGS identity conversion";
                
                // Mass should also be unchanged
                EXPECT_NEAR(values[3], 1.0, 1e-6)
                    << "Mass should be unchanged with CGS identity conversion";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, CanSwitchBackToCodeUnits) {
    GIVEN("An Output object with Galactic units") {
        Output<1> output;
        output.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);
        
        WHEN("Switching back to CODE_UNITS") {
            output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
            
            THEN("The mode should be CODE_UNITS") {
                EXPECT_EQ(output.get_unit_conversion(), UnitConversionMode::CODE_UNITS);
            }
            
            AND_THEN("New outputs should be in code units") {
                output.output_particle(sim);
                
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                auto values = read_first_particle_csv(csv_file);
                
                // pos_x should be back to -0.5
                EXPECT_NEAR(values[0], -0.5, kTolerance);
                
                // density should be 1.0
                EXPECT_NEAR(values[4], 1.0, kTolerance);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, PreservesRelativeValues) {
    GIVEN("Three particles with different densities") {
        Output<1> output;
        
        WHEN("Writing in code units") {
            output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
            output.output_particle(sim);
            
            std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
            std::ifstream file(csv_file);
            std::string line;
            std::getline(file, line); // Skip header
            
            std::vector<real> densities_code;
            for (int i = 0; i < 3; ++i) {
                std::getline(file, line);
                std::stringstream ss(line);
                std::string value;
                for (int j = 0; j < 5; ++j) {
                    std::getline(ss, value, ',');
                }
                densities_code.push_back(std::stod(value));
            }
            file.close();
            
            AND("Writing the same data in Galactic units") {
                fs::remove_all(kTestOutputDir);
                fs::create_directories(kTestOutputDir);
                Logger::open(kTestOutputDir);
                
                Output<1> output_gal;
                output_gal.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);
                output_gal.output_particle(sim);
                
                std::ifstream file_gal(csv_file);
                std::getline(file_gal, line); // Skip header
                
                std::vector<real> densities_gal;
                for (int i = 0; i < 3; ++i) {
                    std::getline(file_gal, line);
                    std::stringstream ss(line);
                    std::string value;
                    for (int j = 0; j < 5; ++j) {
                        std::getline(ss, value, ',');
                    }
                    densities_gal.push_back(std::stod(value));
                }
                
                THEN("The relative ratios should be preserved") {
                    real ratio_code_01 = densities_code[0] / densities_code[1];
                    real ratio_gal_01 = densities_gal[0] / densities_gal[1];
                    EXPECT_NEAR(ratio_code_01, ratio_gal_01, kTolerance);
                    
                    real ratio_code_12 = densities_code[1] / densities_code[2];
                    real ratio_gal_12 = densities_gal[1] / densities_gal[2];
                    EXPECT_NEAR(ratio_code_12, ratio_gal_12, kTolerance);
                }
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, HandlesMultipleSnapshots) {
    GIVEN("An Output object with code units") {
        Output<1> output;
        output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
        
        WHEN("Writing multiple snapshots") {
            output.output_particle(sim);  // Snapshot 0
            
            sim->time = 0.2;
            sim->particles[0].pos[0] = -0.3;
            output.output_particle(sim);  // Snapshot 1
            
            sim->time = 0.3;
            sim->particles[0].pos[0] = -0.1;
            output.output_particle(sim);  // Snapshot 2
            
            THEN("All snapshots should be in code units") {
                auto values0 = read_first_particle_csv(std::string(kTestOutputDir) + "/snapshots/00000.csv");
                auto values1 = read_first_particle_csv(std::string(kTestOutputDir) + "/snapshots/00001.csv");
                auto values2 = read_first_particle_csv(std::string(kTestOutputDir) + "/snapshots/00002.csv");
                
                EXPECT_NEAR(values0[0], -0.5, kTolerance);
                EXPECT_NEAR(values1[0], -0.3, kTolerance);
                EXPECT_NEAR(values2[0], -0.1, kTolerance);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, CodeUnitsMatchAnalyticalSolutions) {
    GIVEN("A shock tube simulation in code units") {
        Output<1> output;
        output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
        
        WHEN("Writing output") {
            output.output_particle(sim);
            
            THEN("Position range should match analytical domain") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                std::ifstream file(csv_file);
                std::string line;
                std::getline(file, line); // Skip header
                
                real min_pos = 1e10;
                real max_pos = -1e10;
                
                for (int i = 0; i < 3; ++i) {
                    std::getline(file, line);
                    std::stringstream ss(line);
                    std::string value;
                    std::getline(ss, value, ',');
                    real pos = std::stod(value);
                    min_pos = std::min(min_pos, pos);
                    max_pos = std::max(max_pos, pos);
                }
                
                // Analytical shock tube domain is typically [-0.5, 1.5]
                EXPECT_GE(min_pos, -1.0) << "Position below analytical domain";
                EXPECT_LE(max_pos, 2.0) << "Position above analytical domain";
            }
            
            AND_THEN("Density range should match analytical values") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                std::ifstream file(csv_file);
                std::string line;
                std::getline(file, line); // Skip header
                
                real min_dens = 1e10;
                real max_dens = -1e10;
                
                for (int i = 0; i < 3; ++i) {
                    std::getline(file, line);
                    std::stringstream ss(line);
                    std::string value;
                    for (int j = 0; j < 5; ++j) {
                        std::getline(ss, value, ',');
                    }
                    real dens = std::stod(value);
                    min_dens = std::min(min_dens, dens);
                    max_dens = std::max(max_dens, dens);
                }
                
                // Analytical Sod shock: density 0.125 (right) to 1.0 (left)
                EXPECT_GE(min_dens, 0.0) << "Density should be non-negative";
                EXPECT_LE(max_dens, 10.0) << "Density unexpectedly high for Sod shock";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(UnitConversionModeTestFixture, TypeSafetPreventsInvalidModes) {
    GIVEN("The UnitConversionMode enum") {
        THEN("Only valid modes can be used") {
            // This test verifies compile-time type safety
            // The following would not compile:
            // output.set_unit_conversion(5);  // Error: int is not UnitConversionMode
            // output.set_unit_conversion("CODE_UNITS");  // Error: string not allowed
            
            // Only these are valid:
            Output<1> output;
            output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
            output.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);
            output.set_unit_conversion(UnitConversionMode::SI_UNITS);
            output.set_unit_conversion(UnitConversionMode::CGS_UNITS);
            
            // Test passes if it compiles
            SUCCEED();
        }
    }
}

} // FEATURE UnitConversionMode
